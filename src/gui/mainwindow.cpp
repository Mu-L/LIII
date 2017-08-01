#include "mainwindow.h"
#include "application.h"

#include <QList>
#include <QPair>
#include <QMimeData>

#include <iostream>
#include <stdio.h>

#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QClipboard>
#include <QDesktopWidget>
#include <QCheckBox>

#include "utilities/credential.h"
#include "utilities/utils.h"
#include "utilities/translation.h"
#include "utilities/associate_app.h"


#include "aboutdialog.h"
#include "add_links.h"
#include "preferences.h"
#include "logindialog.h"
#include "settings_declaration.h"
#include "global_functions.h"
#include "globals.h"
#include "qnamespace.h"
#include "branding.hxx"


#if defined (Q_OS_WIN)
#include <windows.h>
#elif defined (Q_OS_MAC)
#include "darwin/DarwinSingleton.h"
#include "darwin/AppHandler.h"
#endif

#define PROJECT_SITE "http://www." PROJECT_DOMAIN

namespace Tr = utilities::Tr;

using utilities::Credential;

using namespace app_settings;


MainWindow::MainWindow()
    : ui_utils::MainWindowWithTray(nullptr, QIcon(PROJECT_ICON), PROJECT_FULLNAME_TRANSLATION),
      ui(new Ui::MainWindow),
      m_dlManager(nullptr)
      , isAutorun(false)
{
    qApp->setWindowIcon(QIcon(PROJECT_ICON));

    ui->setupUi(this);

    // remove "Stop" action temporary
    ui->stopButton->hide();

    setWindowIcon(QIcon(PROJECT_ICON));
    setIconSize(QSize(48, 48));
    setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);

    setAcceptDrops(true);

    ui->buttonOptions->setText(" ");

    ::Tr::SetTr(this, &QWidget::setWindowTitle, PROJECT_FULLNAME_TRANSLATION);
    ::Tr::SetTr(ui->actionAbout_LIII, &QAction::setText, ABOUT_TITLE, PROJECT_NAME);

    QHBoxLayout* horizontalLayout = new QHBoxLayout(ui->menuBar);
    horizontalLayout->setSpacing(6);
    horizontalLayout->setObjectName(QString::fromUtf8("horizontalMenuBarLayout"));
    horizontalLayout->setContentsMargins(0, 2, 8, 2);

    QSpacerItem* horizSpacer1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalLayout->addItem(horizSpacer1);

    DownloadCollectionModel* m_pModel = &DownloadCollectionModel::instance();

    VERIFY(connect(m_pModel, SIGNAL(signalDeleteURLFromModel(int, DownloadType::Type, int)) , SLOT(refreshButtons())));
    VERIFY(connect(m_pModel, SIGNAL(overallProgress(int)), this, SLOT(onOverallProgress(int))));
    VERIFY(connect(m_pModel, SIGNAL(activeDownloadsNumberChanged(int)), this, SLOT(onActiveDownloadsNumberChanged(int))));

    m_dlManager = new DownloadManager(this);
    VERIFY(connect(m_dlManager, SIGNAL(updateButtons()), this, SLOT(refreshButtons())));
    VERIFY(connect(m_dlManager, SIGNAL(needLogin(utilities::ICredentialsRetriever*)), this, SLOT(needLogin(utilities::ICredentialsRetriever*))));

    ui->listUrls->setItemDelegate(new DownloadCollectionDelegate(this));
    ui->listUrls->setModel(m_pModel);

    ui->actionPaste_Links->setShortcut(QKeySequence::Paste);

    VERIFY(connect(ui->listUrls, SIGNAL(signalOpenFolder(const QString&)), this, SLOT(onButtonOpenFolderClicked(const QString&))));
    VERIFY(connect(ui->listUrls, SIGNAL(signalOpenTorrentFolder(const QString&, const QString&)), this, SLOT(openTorrentDownloadFolder(const QString&, const QString&))));
    VERIFY(connect(ui->listUrls, SIGNAL(signalButtonChangePauseImage(bool, bool, bool, bool)), this, SLOT(onChangePauseCancelState(bool, bool, bool, bool))));
    VERIFY(connect(ui->listUrls, SIGNAL(signalDownloadFinished(const QString&)), this, SLOT(showTrayNotifDwnldFinish(const QString&))));

    VERIFY(connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(on_openTorrent_clicked())));
    VERIFY(connect(ui->actionClose_Link, SIGNAL(triggered()), this, SLOT(onActionCloseLinkClicked())));
    VERIFY(connect(ui->actionExit_Link, SIGNAL(triggered()), this, SLOT(closeApp())));
    VERIFY(connect(ui->actionPaste_Links, SIGNAL(triggered()), this, SLOT(on_buttonPaste_clicked())));
    VERIFY(connect(ui->actionPause_selected, SIGNAL(triggered()), this, SLOT(on_pauseButton_clicked())));
    VERIFY(connect(ui->actionCancel_selected, SIGNAL(triggered()), this, SLOT(on_cancelButton_clicked())));
    VERIFY(connect(ui->actionStart, SIGNAL(triggered()), this, SLOT(on_startButton_clicked())));
    //    VERIFY(connect(ui->actionStop, SIGNAL(triggered()), this, SLOT(on_stopButton_clicked())));

    VERIFY(connect(ui->actionCleanup, SIGNAL(triggered()), this, SLOT(on_clearButton_clicked())));
    VERIFY(connect(ui->actionAbout_LIII, SIGNAL(triggered()), this, SLOT(onAboutClicked())));

    VERIFY(connect(ui->actionStartAllDownloads, SIGNAL(triggered()), ui->listUrls, SLOT(resumeAllItems())));
    VERIFY(connect(ui->actionPauseAllDownloads, SIGNAL(triggered()), ui->listUrls, SLOT(pauseAllItems())));
    //    VERIFY(connect(ui->actionStopAllDownloads, SIGNAL(triggered()), ui->listUrls, SLOT(stopAllItems())));
    VERIFY(connect(ui->buttonOpenFolder, SIGNAL(clicked()), this, SLOT(onButtonOpenFolderClicked())));

#ifdef Q_OS_MAC
    VERIFY(connect(&DarwinSingleton::Instance(), SIGNAL(showPreferences()), this, SLOT(on_buttonOptions_clicked())));
    VERIFY(connect(&DarwinSingleton::Instance(), SIGNAL(showAbout()), this, SLOT(onAboutClicked())));
    VERIFY(connect(&DarwinSingleton::Instance(), SIGNAL(addTorrent(QStringList)), this, SLOT(openTorrent(QStringList))));

    ui->menuFile->menuAction()->setVisible(false);
    ui->menuTools->menuAction()->setVisible(false);
    ui->linkEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->listUrls->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->actionClose_Link->setShortcut(QKeySequence("Cmd+W"));
#endif //Q_OS_MAC

    VERIFY(connect(ui->actionPreferences, SIGNAL(triggered()), this, SLOT(on_buttonOptions_clicked())));

    populateTrayMenu();

    VERIFY(connect(ui->linkEdit, SIGNAL(linksAdd(bool)), ui->lblClearText, SLOT(setVisible(bool))));
    VERIFY(connect(ui->linkEdit, SIGNAL(returnPressed()), this, SLOT(on_buttonStart_clicked())));
    VERIFY(connect(ui->lblClearText, SIGNAL(clicked()), this, SLOT(onlblClearTextCliced())));

    refreshButtons();
}

#if defined(Q_OS_WIN)
static unsigned int TaskBarMsg = TaskBar::InitMessage();

bool MainWindow::winEvent(MSG* msg, long* result)
{
    Q_UNUSED(result);
    if (msg->message == TaskBarMsg)
    {
        m_taskBar.Init(winId());
    }
    return false;
}
#endif

void MainWindow::moveToScreenCenter()
{
    // adjust window size to screen if low resolution
    QRect desktopAvailRect    = QApplication::desktop()->availableGeometry();
    QRect windowDefaultRect    = geometry();
    const QSize screenSize = desktopAvailRect.size();
    if (windowDefaultRect.width() > screenSize.width())
    {
        windowDefaultRect.setWidth(screenSize.width());
    }
    if (windowDefaultRect.height() > screenSize.height())
    {
        windowDefaultRect.setHeight(screenSize.height());
        showMaximized(); // to place window header to the screen as well
    }
    windowDefaultRect.moveCenter(desktopAvailRect.center());

    setGeometry(windowDefaultRect);
}

void MainWindow::showMainWindowAndPerformChecks()
{

    moveToScreenCenter();

    m_dlManager->startLoad();
    VERIFY(connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(prepareToExit())));

    refreshButtons();

    if (!isAutorun)
    {
        show();
        checkDefaultTorrentApplication();
    }
    if (Application* myApp = dynamic_cast<Application*>(qApp))
    {
        myApp->checkFirewallException(this);
    }
}

void MainWindow::checkDefaultTorrentApplication()
{
    if (QSettings().value(ShowAssociateTorrentDialog, ShowAssociateTorrentDialog_Default).toBool()
        && !utilities::isDefaultTorrentApp())
    {
        QMessageBox msgBox(
            QMessageBox::NoIcon,
            ::Tr::Tr(PROJECT_FULLNAME_TRANSLATION),
            ::Tr::Tr(ASSOCIATE_TORRENT_TEXT).arg(PROJECT_NAME),
            QMessageBox::Yes | QMessageBox::No,
            this);
        msgBox.setDefaultButton(QMessageBox::Yes);
        msgBox.setCheckBox(new QCheckBox(::Tr::Tr(DONT_SHOW_THIS_AGAIN)));

        const bool result = msgBox.exec() == QMessageBox::Yes;

        const bool isDontShowMeChecked = msgBox.checkBox()->isChecked();
        QSettings().setValue(ShowAssociateTorrentDialog, !isDontShowMeChecked);
        if (result)
        {
            utilities::setDefaultTorrentApp(winId());
        }
    }
}

void MainWindow::onChangePauseCancelState(bool canPause, bool canResume, bool canCancel, bool canStop)
{
    ui->startButton->setEnabled(canResume);
    ui->pauseButton->setEnabled(canPause);
    ui->stopButton->setEnabled(canStop);
    ui->actionPause_selected->setEnabled(canPause);
    ui->cancelButton->setEnabled(canCancel);
    ui->actionCancel_selected->setEnabled(canCancel);
    ui->actionStart->setEnabled(canResume);
}

void MainWindow::populateTrayMenu()
{
    addTrayMenuItem(TrayMenu::Show);
    addTrayMenuItem(ui->actionPreferences);
    addTrayMenuItem(TrayMenu::Separator);
    addTrayMenuItem(ui->actionStartAllDownloads);
    addTrayMenuItem(ui->actionPauseAllDownloads);
    //addTrayMenuItem(ui->actionStopAllDownloads);
    addTrayMenuItem(TrayMenu::Separator);
    addTrayMenuItem(TrayMenu::Exit);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::onAboutClicked()
{
    raise();
    activateWindow();
    AboutDialog(this).exec();
}

void MainWindow::onActionCloseLinkClicked()
{
    hide();
}

void MainWindow::closeApp()
{
    static bool isrunning = false;
    if (!isrunning)
    {
        isrunning = true;
        if (m_dlManager->isWorking() 
            && QSettings().value(ShowExitWarning, ShowExitWarning_Default).toBool())
        {
            QMessageBox msgBox(
                QMessageBox::NoIcon,
                ::Tr::Tr(PROJECT_FULLNAME_TRANSLATION),
                ::Tr::Tr(EXIT_TEXT).arg(PROJECT_NAME),
                QMessageBox::Yes | QMessageBox::No,
                this);
            msgBox.setDefaultButton(QMessageBox::No);
            msgBox.setCheckBox(new QCheckBox(::Tr::Tr(DONT_SHOW_THIS_AGAIN)));

            if (msgBox.exec() != QMessageBox::Yes)
            {
                isrunning = false;
                return;
            }
            const bool isDontShowMeChecked = msgBox.checkBox()->isChecked();
            QSettings().setValue(ShowExitWarning, !isDontShowMeChecked);
        }
        MainWindowWithTray::closeApp();
        isrunning = false;
    }
}

void MainWindow::prepareToExit()
{
    qDebug() << __FUNCTION__;
    m_dlManager->prepareToExit();
#ifdef Q_OS_WIN
    m_taskBar.Uninit();
#endif
}

void MainWindow::onlblClearTextCliced()
{
    ui->linkEdit->setText(QString());
    refreshButtons();
}

void MainWindow::on_buttonStart_clicked()
{
    if (!ui->linkEdit->text().isEmpty())
    {
        addLinks(utilities::ParseUrls(ui->linkEdit->text()));
    }
    else
    {
        ui->linkEdit->setErrorState();
    }
    ui->linkEdit->setText(QString());
    if (DownloadCollectionModel::instance().rowCount() == 0)
    {
        return;
    }

    m_dlManager->startLoad();
    refreshButtons();
}

void MainWindow::addLinks(QStringList urls)
{
    if (!urls.empty())
    {
        // show links dialog if adding multiple links
        if (urls.size() > 1)
        {
            AddLinks dlg(urls, this);
            if (dlg.exec() != QDialog::Accepted)
            {
                return;
            }
            urls = dlg.urls();
        }

        // add links to model
        m_dlManager->addItemsToModel(urls, DownloadType::Unknown);

        // update UI according model changes
        refreshButtons();
    }
}


void MainWindow::needLogin(utilities::ICredentialsRetriever* icr)
{
    LoginDialog dlg(this);
    Credential cred;
    if (dlg.exec() == QDialog::Accepted)
    {
        cred.login = dlg.login();
        cred.password = dlg.password();
    }
    icr->SetCredentials(cred);
}

template <class Tab_t>
void MainWindow::OpenPreferences(Tab_t tab)
{
    static_assert(std::is_same<Tab_t, Preferences::TAB>::value, "Tab_t must be Preferences::TAB");

    Preferences prefDlg(this, tab);
    VERIFY(connect(&prefDlg, SIGNAL(newPreferencesApply()), m_dlManager, SLOT(siftDownloads())));
    if (prefDlg.exec() == QDialog::Accepted)
    {
#ifdef ALLOW_TRAFFIC_CONTROL
        m_dlManager->setSpeedLimit(global_functions::GetTrafficLimitActual()); // just in case
#endif // ALLOW_TRAFFIC_CONTROL
        m_dlManager->startLoad();
        refreshButtons();
    }
}

void MainWindow::on_buttonOptions_clicked()
{
    raise();
    activateWindow();
    OpenPreferences(Preferences::GENERAL);
}

void MainWindow::onButtonOpenFolderClicked(const QString& filename)
{
    utilities::SelectFile(filename, global_functions::GetVideoFolder());
}

void MainWindow::openTorrentDownloadFolder(const QString& filename, const QString& downloadDirectory)
{
    utilities::SelectFile(filename, downloadDirectory);
}

void MainWindow::on_buttonPaste_clicked()
{
    QClipboard* clipboard = QApplication::clipboard();

    QStringList links = utilities::ParseUrls(clipboard->mimeData()->text());
    if (links.isEmpty())
    {
        QMessageBox msgBox(
            QMessageBox::NoIcon,
            ::Tr::Tr(PROJECT_FULLNAME_TRANSLATION),
            ::Tr::Tr(NO_LINKS_IN_CLIPBOARD),
            QMessageBox::Ok,
            this);
        msgBox.exec();
    }
    else
    {
        addLinks(links);
    }
}

void MainWindow::on_clearButton_clicked()
{
    if (QSettings().value(ShowCleanupWarning, ShowCleanupWarning_Default).toBool())
    {
        QMessageBox msgBox(
            QMessageBox::NoIcon,
            ::Tr::Tr(PROJECT_FULLNAME_TRANSLATION),
            ::Tr::Tr(CLEANUP_TEXT),
            0,
            this);
        msgBox.setCheckBox(new QCheckBox(::Tr::Tr(DONT_SHOW_THIS_AGAIN)));
        QPushButton* removeButton = msgBox.addButton(tr("Remove"), QMessageBox::ActionRole);
        QPushButton* cancelButton = msgBox.addButton(QMessageBox::Cancel);

        msgBox.exec();
        if (msgBox.clickedButton() != removeButton)
        {
            return;
        }
        const bool isDontShowMeChecked = msgBox.checkBox()->isChecked();
        QSettings().setValue(ShowCleanupWarning, !isDontShowMeChecked);
    }

    DownloadCollectionModel::instance().deleteALLFinished();
    refreshButtons();
}

void MainWindow::on_startButton_clicked()
{
    ui->listUrls->startDownloadItem();
}

void MainWindow::on_pauseButton_clicked()
{
    ui->listUrls->pauseDownloadItem();
}

//void MainWindow::on_stopButton_clicked()
//{
//    ui->listUrls->stopDownloadItem();
//}

void MainWindow::on_cancelButton_clicked()
{
    ui->listUrls->deleteSelectedRows();
    refreshButtons();
}

void MainWindow::refreshButtons()
{
    const bool isCompletedItems 
        = DownloadCollectionModel::instance().findItem([](const TreeItem * ti) {return ti->isCompleted();}) != nullptr;
    ui->lblClearText->setVisible(!ui->linkEdit->text().isEmpty());
    ui->clearButton->setEnabled(isCompletedItems);
    ui->actionCleanup->setEnabled(isCompletedItems);
    ui->listUrls->getUpdateItem();
}

bool MainWindow::setError(const QString& strHeader, const QString& strErrorText)
{
    bool isNotEmptyHeader = !strHeader.isEmpty();
    if (isNotEmptyHeader)
    {
        QMessageBox msgBox(
            QMessageBox::Critical,
            ::Tr::Tr(PROJECT_FULLNAME_TRANSLATION),
            strHeader,
            QMessageBox::Ok,
            this);
        msgBox.setInformativeText(strErrorText);
        msgBox.exec();
    }
    return isNotEmptyHeader;
}

void MainWindow::languageChange()
{
    // will be called after call of RetranslateApp
    ::Tr::RetranslateAll(this);
    ui->retranslateUi(this);
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        languageChange();
    }
}


void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat("text/plain") || event->mimeData()->hasFormat("text/uri-list"))
    {
        event->acceptProposedAction();
    }
}

static QString extractLinkFromFile(const QString& fn)
{
    QString link;

    if (QFile::exists(fn))
    {
        QSettings settings(fn, QSettings::IniFormat);
        link = settings.value("InternetShortcut/URL").toString();
    }
    return link;
}

void MainWindow::dropEvent(QDropEvent* event)
{
    QString text;
    QRegExp intrestedDataRx;

    if (event->mimeData()->hasHtml())
    {
        text = event->mimeData()->html();
        intrestedDataRx = QRegExp("(http|ftp|file|magnet|[a-z]):[^\"'<>\\s\\n]+", Qt::CaseInsensitive);
    }
    else if (event->mimeData()->hasText())
    {
        text = event->mimeData()->text();
        intrestedDataRx = QRegExp("(http|ftp|file|magnet|[a-z]):[^\\s\\n]+", Qt::CaseInsensitive);
    }
    else if (event->mimeData()->hasFormat("text/uri-list"))
    {
        QByteArray data = event->mimeData()->data("text/uri-list");
        text = data;
        intrestedDataRx = QRegExp("(http|ftp|file|magnet|[a-z]):[^\\s\\n]+", Qt::CaseInsensitive);
    }
    else
    {
        return;
    }

    qDebug() << "Some data dropped to program. Trying to manage it.";

    int pos = 0;
    QUrl url;
    QStringList linksForDownload;
    while ((pos = intrestedDataRx.indexIn(text, pos)) != -1)
    {
        QString someLink = intrestedDataRx.cap(0);
        QString typeOfLink = intrestedDataRx.cap(1);
        qDebug() << QString(PROJECT_NAME) + " takes " + someLink;
        pos += intrestedDataRx.matchedLength();


        if (typeOfLink == "file")
        {
            someLink = QUrl(QUrl::fromPercentEncoding(someLink.toLatin1())).toLocalFile();
            DownloadType::Type fileType = DownloadType::determineType(someLink);
            switch (fileType)
            {
            case DownloadType::TorrentFile:
                break;
            case DownloadType::LocalFile:
                someLink = extractLinkFromFile(someLink);
                break;
            default:
                Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown file type");
                continue;
            }
        }
        linksForDownload << someLink;
    }
    addLinks(utilities::ParseUrls(linksForDownload.join("\n")));

    event->acceptProposedAction();
}

void MainWindow::showHideNotify()
{
    if (QSettings().value(ShowSysTrayNotifications, ShowSysTrayNotifications_Default).toBool()
        && QSettings().value(ShowSysTrayNotificationOnHide, ShowSysTrayNotificationOnHide_Default).toBool())
    {
        QSettings().setValue(ShowSysTrayNotificationOnHide, false);
        showTrayMessage(tr("%1 continues running. Click this button to open it").arg(::Tr::Tr(PROJECT_FULLNAME_TRANSLATION)));
    }
}

void MainWindow::showTrayNotifDwnldFinish(const QString& str)
{

    if (QSettings().value(ShowSysTrayNotifications, ShowSysTrayNotifications_Default).toBool())
    {
#ifdef Q_OS_WIN
        showTrayMessage(tr("File \"%1\" was downloaded").arg(str));
#elif defined(Q_OS_MAC)
        Darwin::showNotification(tr("File downloaded"), tr("File \"%1\" was downloaded").arg(str));
#endif //Q_OS_WIN
    }
}


void MainWindow::on_openTorrent_clicked()
{
    QString startPath = utilities::getPathForDownloadFolder();

    QStringList filenames = QFileDialog::getOpenFileNames(this, tr("Select a Torrent to open"), startPath, "Torrent Files(*.torrent);;All(*.*)");
    if (!filenames.empty())
    {
        addLinks(filenames);
    }
}


void MainWindow::keyPressEvent(QKeyEvent* event)
{
#ifdef DEVELOPER_FEATURES
    if (event->key() == Qt::Key_F5)
    {
        qDebug() << "apply custom css";
#ifndef Q_OS_MAC
        QFile file(QApplication::applicationDirPath() + "/../../../resources/LIII/style.css");
        if (file.exists() && file.open(QIODevice::ReadOnly))
        {
            setStyleSheet(file.readAll());
            file.close();
        }
#else
        QFile file(QApplication::applicationDirPath() + "/../../../../../../resources/LIII/style.css");
        QFile macfile(QApplication::applicationDirPath() + "/../../../../../../resources/LIII/macstyle.css");
        if (file.exists() && macfile.exists() && file.open(QIODevice::ReadOnly) && macfile.open(QIODevice::ReadOnly))
        {
            setStyleSheet(file.readAll() + "\n" + macfile.readAll());
            file.close();
            macfile.close();
        }

#endif //!Q_OS_MAC
    }
#endif
    QMainWindow::keyPressEvent(event);
}

#ifdef Q_OS_MAC
QString MainWindow::findApplicationPath(const QString& appBrand)
{
    QString findCommandLine = QString("echo \"set brand to \\\"%1\\\" \n\
set brandBundleID to id of application brand \n\
try \n\
set brandFiles to paragraphs of (do shell script \\\"mdfind \\\\\\\"kMDItemCFBundleIdentifier='\\\" & brandBundleID & \\\"'\\\\\\\"\\\") \n\
on error errText number errNum \n\
set brandFiles to {} \n\
end try \n\
do shell script \\\"echo \\\" & item 1 in brandFiles\" | /usr/bin/osascript"
                                     ).arg(appBrand);

    FILE* pipe = popen(findCommandLine.toAscii().data(), "r");
    if (!pipe)
    {
        return QString();
    }
    else
    {
        char buffer[128];
        std::string result = "";
        while (!feof(pipe))
        {
            if (fgets(buffer, 128, pipe) != NULL)
            {
                result += buffer;
            }
        }
        pclose(pipe);
        return QString::fromUtf8(result.c_str()).replace("\n", "");
    }
}
#endif //Q_OS_MAC

void MainWindow::openTorrent(QStringList magnetUrls)
{
    addLinks(magnetUrls);
}

void MainWindow::onOverallProgress(int progress)
{
#ifdef Q_OS_WIN
    m_taskBar.setProgress(progress);
#elif defined(Q_OS_MAC)
    Darwin::setOverallProgress(progress);
#endif
}

void MainWindow::onActiveDownloadsNumberChanged(int number)
{
#ifdef Q_OS_WIN
#elif defined(Q_OS_MAC)
    Darwin::setDockBadge(number);
#endif
}

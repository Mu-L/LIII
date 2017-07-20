#include "tabbeddialogcombo.h"

#include <QKeyEvent>

namespace ui_utils
{

TabbedDialogCombo::TabbedDialogCombo(QWidget* parent)
    : QComboBox(parent)
{
    installEventFilter(this);
}


bool TabbedDialogCombo::eventFilter(QObject* receiver, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = (QKeyEvent*)event;
        if (keyEvent->matches(QKeySequence::NextChild))
        {
            if (QTabWidget* tabWidget = getTabParent())
            {
                tabWidget->setCurrentIndex((tabWidget->currentIndex() + 1) % tabWidget->count());
            }

            return true;
        }
        else if (keyEvent->matches(QKeySequence::PreviousChild))
        {
            if (QTabWidget* tabWidget = getTabParent())
            {
                int newIndex = tabWidget->currentIndex() - 1;
                if (newIndex < 0)
                {
                    newIndex = tabWidget->count() - 1;
                }
                tabWidget->setCurrentIndex(newIndex);
            }

            return true;
        }
    }
    return false;
}

QTabWidget* TabbedDialogCombo::getTabParent()
{
    for (QObject* parentObj = parent(); parentObj != 0; parentObj = parentObj->parent())
    {
        QTabWidget* tabWidget = qobject_cast<QTabWidget*>(parentObj);
        if (tabWidget != 0)
        {
            return tabWidget;
        }
    }

    return 0;
}

void TabbedDialogCombo::showPopup()
{
    QStyle* pStyle = style();
    QStyleOptionComboBox opt;
    opt.initFrom(this);
    QRect rc_boxPopup = pStyle->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxListBoxPopup, this);
    QRect rc_boxArrow = pStyle->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxArrow, this);

    QWidget* popup = this->findChild<QFrame*>();
    popup->setMaximumWidth(rc_boxPopup.width() - rc_boxArrow.width());
    QComboBox::showPopup();
}

}

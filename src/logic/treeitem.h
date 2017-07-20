#pragma once

#include "utilities/errorcode.h"

#include "downloadtype.h"

#include <QStringList>
#include <QUrl>
#include <QDateTime>

#include <utility>

typedef int ItemID;
const ItemID nullItemID = -1;

class CopyableQObject : public QObject
{
public:
    CopyableQObject() : QObject() {}
    CopyableQObject(const CopyableQObject&) {}
    CopyableQObject& operator =(const CopyableQObject&) { return *this; }
};

#define DECLARE_Q_PROPERTY(prop, read, write) \
    public: \
        prop() const { return read##_; } \
        void write(prop) { read##_ = std::move(read); } \
    private: \
        prop##_ {};

#define DECLARE_Q_PROPERTY_ADAPTOR(params) DECLARE_Q_PROPERTY params

class ItemDC
    : public CopyableQObject
{
    Q_OBJECT
    Q_ENUMS(eSTATUSDC)
    Q_ENUMS(eITEMPLAY)
public:
    ItemDC()
        : m_ID(nullItemID)
        , m_eStatus(eQUEUED)
        , m_dSpeed(0.0)
        , m_dSpeedUpload(0.0)
        , m_percentDownload(0)
        , m_iSize(0)
        , m_iSizeCurrDownl(0)
        , m_iWaitingTime(0)
        , m_errorCode(utilities::ErrorCode::eNOTERROR)
        , m_priority(0)
        , m_downloadType(DownloadType::RemoteUrl)
    {};

    enum eSTATUSDC {
        eWAITING,
        eQUEUED,
        eDOWNLOADING,
        eCONNECTING,
        eFINISHED,
        ePAUSED,
        eERROR,
        eROOTSTATUS,
        eSEEDING,
        eSTALLED,
        eSTARTING,
        eSTOPPED,
        eUNKNOWN
    };
    enum eITEMPLAY {
        ePLAY_PREPARE,
        ePLAYREADY,
        ePLAYIMPOSSIBLE,
        eDOWNLOAD_FAILED,
        ePLAYQUEUED,
        ePLAYPAUSED,
        ePLAY_ENABLED = 10,
        ePLAY_DISABLED,
        eFILE_DISABLED,
        eFILE_ENABLED,
        eFOLDER,
        eFOLDER_DISABLED,
        eWAITING_INFO
    };

    bool isValid() const { return m_ID != nullItemID; }
    explicit operator bool() const { return isValid(); }

    inline ItemID getID() const { return m_ID; }
    void setID(ItemID val) { m_ID = val; }

#ifndef Q_MOC_RUN
#define READ ,
#define WRITE ,
#undef Q_PROPERTY
#define Q_PROPERTY(...) DECLARE_Q_PROPERTY_ADAPTOR((__VA_ARGS__))
#endif // Q_MOC_RUN

    Q_PROPERTY(QString initialURL READ initialURL WRITE setInitialURL)
    Q_PROPERTY(QString actualURL READ actualURL WRITE setActualURL)
    Q_PROPERTY(QString source READ source WRITE setSource)
    Q_PROPERTY(QString downloadedFileName READ downloadedFileName WRITE setDownloadedFileName)
    Q_PROPERTY(QStringList extractedFileNames READ extractedFileNames WRITE setExtractedFileNames)
    Q_PROPERTY(QStringList torrentFilesPriorities READ torrentFilesPriorities WRITE setTorrentFilesPriorities)
    Q_PROPERTY(QString sizeForView READ sizeForView WRITE setSizeForView)
    Q_PROPERTY(QString hash READ hash WRITE setHash)
    Q_PROPERTY(QString torrentSavePath READ torrentSavePath WRITE setTorrentSavePath)
    Q_PROPERTY(QString errorDescription READ errorDescription WRITE setErrorDescription)

#undef Q_PROPERTY
#define Q_PROPERTY(text)
#undef READ
#undef WRITE

public:
    Q_PROPERTY(int status READ getStatus WRITE setStatusEx)
    inline eSTATUSDC getStatus() const { return m_eStatus; }
    void setStatus(eSTATUSDC val)
    {
        if (m_eStatus != val)
        {
            m_statusLastChanged = QDateTime::currentDateTimeUtc();
            m_eStatus = val;
        }
    }

    QDateTime statusLastChanged() const { return m_statusLastChanged; }

    float getSpeed() const { return m_dSpeed; }
    void setSpeed(float val) { m_dSpeed = val; }

    float getSpeedUpload() const { return m_dSpeedUpload; }
    void setSpeedUpload(float val) { m_dSpeedUpload = val; }

    Q_PROPERTY(int percentDWL READ getPercentDownload WRITE setPercentDownload NOTIFY percentDownloadChanged)
    int getPercentDownload() const { return m_percentDownload; }
    void setPercentDownload(int val) { m_percentDownload = val; emit percentDownloadChanged(val); }

    Q_PROPERTY(qint64 size READ getSize WRITE setSize)
    qint64 getSize() const { return m_iSize; }
    void setSize(qint64 val);

    void setExtractedFileName(const QString& val);

    Q_PROPERTY(qint64 sizeCurrDownl READ getSizeCurrDownl WRITE setSizeCurrDownl)
    qint64 getSizeCurrDownl() const { return m_iSizeCurrDownl; }
    void setSizeCurrDownl(qint64 val);

    int getWaitingTime() const { return m_iWaitingTime; }
    void setWaitingTime(int val) { m_iWaitingTime = val; }

    Q_PROPERTY(QString downloadType READ getDownloadTypeStr WRITE setDownloadTypeStr)
    DownloadType::Type getDownloadType() const { return m_downloadType; }
    void setDownloadType(DownloadType::Type val) { m_downloadType = val; }

    utilities::ErrorCode::ERROR_CODES getErrorCode() const { return m_errorCode; }
    void setErrorCode(utilities::ErrorCode::ERROR_CODES val) { m_errorCode = val; }

    int priority()const {return m_priority;}
Q_SIGNALS:
    void percentDownloadChanged(int);

public:
    bool isCompleted() const
    {
        const ItemDC::eSTATUSDC st(getStatus());
        return
            st == ItemDC::eFINISHED ||
            st == ItemDC::eSEEDING  ||
            st == ItemDC::eERROR ;
    }

protected:
    void setStatusEx(int val);
    QString getDownloadTypeStr() const;
    void setDownloadTypeStr(QString const& val);

    ItemID m_ID;
    eSTATUSDC m_eStatus;
    double m_dSpeed;
    double m_dSpeedUpload;
    int m_percentDownload;
    qint64 m_iSize;
    qint64 m_iSizeCurrDownl;
    int m_iWaitingTime;

    QString m_archive1stVolumeFilename;
    utilities::ErrorCode::ERROR_CODES m_errorCode;

    int m_priority;

    DownloadType::Type m_downloadType;

    QDateTime m_statusLastChanged;
}; // class ItemDC

class TreeItem : public ItemDC
{
    Q_OBJECT
public:
    TreeItem(const QString& a_url = QString(), TreeItem* a_parent = 0);
    virtual ~TreeItem();

    void appendChild(TreeItem* child);
    TreeItem* child(int row);
    int childCount() const;

    int row() const;
    TreeItem* parent() const;
    int lastIndexOf(TreeItem* a_child) const;
    bool removeChildItem(TreeItem* a_item);
    void removeALLChild();

    bool insertChildren(int position, int count, int columns);
    bool removeChildren(int position, int count);

    template <typename Pr>
    TreeItem* findItem(Pr pr);

    TreeItem* findItemByID(ItemID);
    TreeItem* findItemByURL(const QString&);
    TreeItem* findItemByURL(const QUrl& url, QString(QUrl::*url2str)()const);

    Q_PROPERTY(QObjectList childItems READ getChildItems WRITE setChildItems)
    QObjectList getChildItems() const;
    void setChildItems(const QObjectList& items);
    template<class Fn_t> void forAll(Fn_t fn);

    // for use as temp object
    void detachChildren() {childItems.clear();}

    void setPriority(int priority) {m_priority = priority;}
    static int currentCounter() { return l_count; }

    bool canPause()const
    {
        const ItemDC::eSTATUSDC st(getStatus());
        return !(st == ItemDC::ePAUSED || st == ItemDC::eSTOPPED || st == ItemDC::eERROR || st == ItemDC::eFINISHED);
    }

    bool canResume()const
    {
        const ItemDC::eSTATUSDC st(getStatus());
        const bool isTorrentDownload(DownloadType::isTorrentDownload(getDownloadType()));
        return (st == ItemDC::ePAUSED  || st == ItemDC::eSTOPPED || st == ItemDC::eERROR 
            || (st == ItemDC::eFINISHED && isTorrentDownload) || (st == ItemDC::eERROR && isTorrentDownload));
    }

    bool canCancel()const
    {
        const ItemDC::eSTATUSDC st(getStatus());
        return st != ItemDC::eFINISHED && st != ItemDC::eSEEDING;
    }

private:
    static int l_count;

    QList<TreeItem*> childItems;
    TreeItem* parentItem;
}; // class TreeItem


template<class Fn_t>
void TreeItem::forAll(Fn_t fn)
{
    fn(*this);
    Q_FOREACH(TreeItem * ti, childItems)
    {
        ti->forAll(fn);
    }
}

template <typename Pr>
TreeItem* TreeItem::findItem(Pr pr)
{
    if (pr(this))
    {
        return this;
    }

    for (int i = 0; i < childItems.count(); i++)
    {
        if (TreeItem* l_child = childItems[i]->findItem(pr))
        {
            return l_child;
        }
    }

    return nullptr;
}


QString itemDCStatusToString(const ItemDC::eSTATUSDC status);

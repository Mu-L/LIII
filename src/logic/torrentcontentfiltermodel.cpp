#include "torrentcontentfiltermodel.h"
#include "torrentcontentmodel.h"

TorrentContentFilterModel::TorrentContentFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent), m_model(new TorrentContentModel(this))
{
    VERIFY(connect(m_model, SIGNAL(filteredFilesChanged()), this, SIGNAL(filteredFilesChanged())));
    setSourceModel(m_model);
    // Filter settings
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setFilterKeyColumn(TorrentContentModelItem::COL_NAME);
    setFilterRole(Qt::DisplayRole);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

TorrentContentFilterModel::~TorrentContentFilterModel()
{
    delete m_model;
}

TorrentContentModel* TorrentContentFilterModel::model() const
{
    return m_model;
}

TorrentContentModelItem::FileType TorrentContentFilterModel::getType(const QModelIndex& index) const
{
    return m_model->getType(mapToSource(index));
}

QModelIndex TorrentContentFilterModel::parent(const QModelIndex& child) const
{
    if (!child.isValid()) { return QModelIndex(); }
    QModelIndex sourceParent = m_model->parent(mapToSource(child));
    if (!sourceParent.isValid()) { return QModelIndex(); }
    return mapFromSource(sourceParent);
}

bool TorrentContentFilterModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    if (m_model->getType(m_model->index(source_row, 0, source_parent)) == TorrentContentModelItem::FOLDER)
    {
        // always accept folders, since we want to filter their children
        return true;
    }
    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

void TorrentContentFilterModel::selectAll()
{
    for (int i = 0; i < rowCount(); ++i)
    {
        setData(index(i, 0), Qt::Checked, Qt::CheckStateRole);
    }
    emit dataChanged(index(0, 0), index(rowCount(), columnCount()));
}

void TorrentContentFilterModel::selectNone()
{
    for (int i = 0; i < rowCount(); ++i)
    {
        setData(index(i, 0), Qt::Unchecked, Qt::CheckStateRole);
    }
    emit dataChanged(index(0, 0), index(rowCount(), columnCount()));
}

TorrentContentModelItem* TorrentContentFilterModel::getTorrentContentModelItem(const QModelIndex& index)const
{
    return m_model->getTorrentContentModelItem(mapToSource(index));
}

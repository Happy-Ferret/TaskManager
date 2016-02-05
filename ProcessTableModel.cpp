#include "ProcessTableModel.h"

ProcessTableModel::ProcessTableModel(QObject *parent)
    :QAbstractTableModel(parent)
{
    sortColumn = -1;
    sortOrder = Qt::AscendingOrder;

    // Set maximum property
    maxProperty << 0 << 0 << 100 << 1024 * 128 << 50 << 1;
    refresh();
}


void ProcessTableModel::refresh()
{
    emit layoutAboutToBeChanged();

    // Iterate through /proc directory to find new process
    for(const QString entry : QDir("/proc").entryList(QDir::NoDotAndDotDot | QDir::Dirs))
    {
        bool isProc = false;
        int uid = entry.toInt(&isProc);

        // If it is a new process
        if(isProc && !pidSet.contains(uid))
        {
            processList.append(new Process(uid, this));
            pidSet.insert(uid);
        }
    }

    // Iterate through current process list to refresh info
    // and remove those have already been killed
    for(Process * process: processList)
    {
        // If process doesn't exist anymore
        if(!process->refresh())
        {
            pidSet.remove(process->property(Process::ID).toUInt());
            processList.removeOne(process);
            delete process;
        }
    }

    sortByColumn(sortColumn, sortOrder);
    emit layoutChanged();
}

int ProcessTableModel::rowCount(const QModelIndex & parent) const
{
    if(parent.isValid())
        return 0;
    else
        return processList.size();
}

int ProcessTableModel::columnCount(const QModelIndex & parent) const
{
    if(parent.isValid())
        return 0;
    else
        return Process::PropertyCount;
}

ProcessTableModel::~ProcessTableModel()
{
    for(Process * process: processList)
        delete process;
}

QVariant ProcessTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        if (role == Qt::DisplayRole)
        {
            switch(section)
            {
            case Process::ProcessName:
                return QString("Process Name");
            case Process::ID:
                return QString("PID");
            case Process::CPUUsage:
                return QString("CPU");
            case Process::MemoryUsage:
                return QString("Memory");
            case Process::DiskUsage:
                return QString("Disk");
            case Process::NetworkUsage:
                return QString("Network");
            default:
                return QVariant();
            }
        }
        else if (role == Qt::TextAlignmentRole)
        {
            if(section == 0)
                return (int)Qt::AlignBottom | (int)Qt::AlignLeft;
            else if(section == 1)
                return (int)Qt::AlignBottom | (int)Qt::AlignHCenter;
            else
                return (int)Qt::AlignBottom | (int)Qt::AlignRight;
        }
    }
    return QVariant();
}

QVariant ProcessTableModel::data(const QModelIndex & index, int role) const
{
    if(!index.isValid())
        return QVariant();

    Process * process = processList[index.row()];
    if(role == Qt::DisplayRole)
    {
        switch(index.column())
        {
        case Process::ProcessName:
            return process->property(Process::ProcessName);
        case Process::ID:
            return process->property(Process::ID);
        case Process::CPUUsage:
            return process->property(Process::CPUUsage).toString() + " %";
        case Process::MemoryUsage:
        {
            unsigned int memory = process->property(Process::MemoryUsage).toUInt();
            if(memory < 1024)
                return QString::number(memory) + " KB";
            else if(memory < 1024 * 1024)
                return QString::number(memory / 1024, 'f', 1) + " MB";
            else
                return QString::number(memory / (1024 * 1024), 'f', 1) + " GB";
        }
        case Process::DiskUsage:
            return process->property(Process::DiskUsage).toString() + " MB/Sec";
        case Process::NetworkUsage:
            return process->property(Process::NetworkUsage).toString() + " Mbps";
        default:
            return QString("None");
        }
    }
    else if (role == Qt::BackgroundRole)
    {
        if(index.column() > 1)
        {
            int level =  process->property(index.column()).toFloat() / (maxProperty.at(index.column()) / 5);
            level = level > 4 ? 4 : level;
            return QBrush(QColor(255, 198, 61, 80 + 100 * ((float)level / 4)));
        }

        return QVariant();
    }
    else if (role == Qt::TextAlignmentRole)
    {
        switch(index.column())
        {
        case Process::ProcessName:
            return (int)Qt::AlignLeft | (int)Qt::AlignVCenter;
        case Process::ID:
            return (int)Qt::AlignHCenter | (int)Qt::AlignVCenter;
        default:
            return (int)Qt::AlignRight | (int)Qt::AlignVCenter;
        }
    }
    return QVariant();
}

void ProcessTableModel::sortByColumn(int column, Qt::SortOrder order)
{
    if(column < 0 || column >= Process::PropertyCount)
        return;

    sortColumn = column;
    sortOrder = order;
    sort(column, order);
}

void ProcessTableModel::sort(int column, Qt::SortOrder order)
{
    layoutAboutToBeChanged();
    if(order == Qt::AscendingOrder)
        std::sort(processList.begin(),processList.end(),
              [=](Process * left, Process * right)
        {
            // If sorted by process name
            if(column == Process::ProcessName)
                return left->property(Process::ProcessName).toString().compare(right->property(Process::ProcessName).toString(), Qt::CaseInsensitive) < 0;
            else
                return left->property(column).toFloat() < right->property(column).toFloat();
        });
    else
        std::sort(processList.begin(),processList.end(),
              [=](Process * left, Process * right)
        {
            // If sorted by process name
            if(column == 0)
                return left->property(Process::ProcessName).toString().compare(right->property(Process::ProcessName).toString(), Qt::CaseInsensitive) > 0;
            else
                return left->property(column).toFloat() > right->property(column).toFloat();
        });

    layoutChanged();
}

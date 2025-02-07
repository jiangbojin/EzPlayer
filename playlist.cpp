#include <QDir>

#include "playlist.h"
#include "ui_playlist.h"

#include "globalhelper.h"

#include "easylogging++.h"

Playlist::Playlist(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Playlist)
{
    ui->setupUi(this);
}

Playlist::~Playlist()
{
    savePlayList();
    delete ui;
}

bool Playlist::Init()
{
    //ui层添加文件 并且绑定实现槽函数
    if (ui->List->Init() == false) {
        return false;
    }
    //读取文件数据 并设置path以及标题xxx.mp4
    if (InitUi() == false) {
        return false;
    }
    //连接 UI 元素的信号与槽函数
    if (ConnectSignalSlots() == false) {
        return false;
    }
    //接收拖拽操作，将文件拖放到播放列表中,从而添加新的视频到播放列表
    setAcceptDrops(true);
    return true;
}
///
/// 负责初始化播放列表的用户界面
/// \return
///
bool Playlist::InitUi()
{
    ui->List->clear();
    QStringList strListPlaylist;
    GlobalHelper::GetPlaylist(strListPlaylist);

    // 设置样式表
    QString styleSheet = R"(
        QTabWidget {
            background-color: #ffffff;
        }
        QTabWidget::pane {
            border: 1px solid #e8e8e8;
            border-radius: 8px;
            background: white;
        }
        QTabBar::tab {
            background: #f5f5f5;
            color: #666666;
            padding: 8px 16px;
            border: 1px solid #e8e8e8;
            border-bottom: none;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
            min-width: 80px;
        }
        QTabBar::tab:selected {
            background: white;
            color: #4a90e2;
            border-bottom: 2px solid #4a90e2;
        }
        QTabBar::tab:hover:!selected {
            background: #fafafa;
            color: #40a9ff;
        }
        MediaList {
            background-color: #ffffff;
            border: 1px solid #e8e8e8;
            border-radius: 8px;
            padding: 4px;
            outline: none;
        }
        MediaList::item {
            color: #333333;
            padding: 8px 12px;
            border-radius: 4px;
            margin: 2px 4px;
        }
        MediaList::item:hover {
            background: #f5f5f5;
        }
        MediaList::item:selected {
            background: #e6f7ff;
            color: #4a90e2;
            border: none;
        }
        QScrollBar:vertical {
            border: none;
            background: #f5f5f5;
            width: 8px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical {
            background: #ccc;
            border-radius: 4px;
            min-height: 20px;
        }
        QScrollBar::handle:vertical:hover {
            background: #999;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: none;
        }
    )";
    this->setStyleSheet(styleSheet);

    for (QString strVideoFile : strListPlaylist) {
        QFileInfo fileInfo(strVideoFile);
        {
            QListWidgetItem *pItem = new QListWidgetItem(ui->List);
            pItem->setData(Qt::UserRole, QVariant(fileInfo.filePath()));  // 用户数据
            pItem->setText(fileInfo.fileName());  // 显示文本
            pItem->setToolTip(fileInfo.filePath()); // 完整路径提示
            ui->List->addItem(pItem);
        }
    }

    if (strListPlaylist.length() > 0) {
        ui->List->setCurrentRow(0);
    }

    return true;
}

bool Playlist::ConnectSignalSlots()
{
    QList<bool> listRet;
    bool bRet;
    bRet = connect(ui->List, &MediaList::SigAddFile, this, &Playlist::OnAddFile);
    listRet.append(bRet);
    //保证所有文件连接槽成功
    for (bool bReturn : listRet) {
        if (bReturn == false) {
            return false;
        }
    }
    return true;
}

void Playlist::savePlayList()
{
    QStringList strListPlayList;
    for (int i = 0; i < ui->List->count(); i++) {
        strListPlayList.append(ui->List->item(i)->toolTip());
    }
    GlobalHelper::SavePlaylist(strListPlayList);
}

void Playlist::on_List_itemDoubleClicked(QListWidgetItem *item)
{
    LOG(INFO) << "play list double click: " << item->data(Qt::UserRole).toString().toStdString();
    emit SigPlay(item->data(Qt::UserRole).toString().toStdString());
    m_nCurrentPlayListIndex = ui->List->row(item);
    ui->List->setCurrentRow(m_nCurrentPlayListIndex);
}

bool Playlist::GetPlaylistStatus()
{
    if (this->isHidden()) {
        return false;
    }
    return true;
}

int Playlist::GetCurrentIndex()
{
    return m_nCurrentPlayListIndex;
}

std::string Playlist::GetCurrentUrl()
{
    std::string url;
    if(ui->List->count() > 0)  {
        QListWidgetItem *item = ui->List->item(m_nCurrentPlayListIndex);
        url = item->data(Qt::UserRole).toString().toStdString();
    }
    return url;
}

std::string Playlist::GetPrevUrlAndSelect()
{
    std::string url;
    if(ui->List->count() > 0) {
        m_nCurrentPlayListIndex -= 1;
        if(m_nCurrentPlayListIndex < 0) {
            m_nCurrentPlayListIndex = ui->List->count() - 1;
        }
        QListWidgetItem *item = ui->List->item(m_nCurrentPlayListIndex);
        url = item->data(Qt::UserRole).toString().toStdString();
        ui->List->setCurrentRow(m_nCurrentPlayListIndex);       // 选中当前行
    }
    return url;
}

std::string Playlist::GetNextUrlAndSelect()
{
    std::string url;
    if(ui->List->count() > 0) {
        m_nCurrentPlayListIndex += 1;
        if(m_nCurrentPlayListIndex >= ui->List->count()) {
            m_nCurrentPlayListIndex = 0;
        }
        QListWidgetItem *item = ui->List->item(m_nCurrentPlayListIndex);
        url = item->data(Qt::UserRole).toString().toStdString();
        ui->List->setCurrentRow(m_nCurrentPlayListIndex);       // 选中当前行
    }
    return url;
}

void Playlist::AddNetworkUrl(QString network_url)
{
    OnAddFile(network_url);
}

void Playlist::OnRequestPlayCurrentFile()
{
    if(ui->List->count() > 0) {     // 有文件才会触发请求播放
        on_List_itemDoubleClicked(ui->List->item(m_nCurrentPlayListIndex));
        ui->List->setCurrentRow(m_nCurrentPlayListIndex);   // 选中当前行
    }
}

void Playlist::OnAddFile(QString strFileName)
{
    bool bSupportMovie = strFileName.endsWith(".mkv", Qt::CaseInsensitive) ||
                         strFileName.endsWith(".rmvb", Qt::CaseInsensitive) ||
                         strFileName.endsWith(".mp4", Qt::CaseInsensitive) ||
                         strFileName.endsWith(".avi", Qt::CaseInsensitive) ||
                         strFileName.endsWith(".flv", Qt::CaseInsensitive) ||
                         strFileName.endsWith(".wmv", Qt::CaseInsensitive) ||
                         strFileName.endsWith(".ts", Qt::CaseInsensitive) ||
                         strFileName.endsWith(".3gp", Qt::CaseInsensitive) ||
                         strFileName.endsWith(".wav", Qt::CaseInsensitive) ||
                         strFileName.endsWith(".mp3", Qt::CaseInsensitive) ||
                         strFileName.endsWith(".aac", Qt::CaseInsensitive) ||
                         strFileName.startsWith("rtp://", Qt::CaseInsensitive) ||
                         strFileName.startsWith("udp://", Qt::CaseInsensitive) ||
                         strFileName.startsWith("rtmp://", Qt::CaseInsensitive) ||
                         strFileName.startsWith("https://", Qt::CaseInsensitive) ||
                         strFileName.startsWith("http://", Qt::CaseInsensitive);
    if (!bSupportMovie) {// 如果不支持该文件格式,直接返回
        return;
    }
    // 获取文件信息
    QFileInfo fileInfo(strFileName);
    // 在播放列表中查找是否已经存在该文件
    QList<QListWidgetItem *> listItem = ui->List->findItems(fileInfo.fileName(), Qt::MatchExactly);

    QListWidgetItem *pItem = nullptr;
    // 如果列表中不存在该文件,创建新的列表项
    if (listItem.isEmpty()) {
        pItem = new QListWidgetItem(ui->List);
        pItem->setData(Qt::UserRole, QVariant(fileInfo.filePath()));  // 用户数据
        pItem->setText(fileInfo.fileName());  // 显示文件名
        pItem->setToolTip(fileInfo.filePath()); // 完整文件路径
        ui->List->addItem(pItem);
        // 加入成功则选中行数
    } else {
        pItem = listItem.at(0);
    }
    m_nCurrentPlayListIndex = ui->List->row(pItem);     // 设置当前选中的列表项
    ui->List->setCurrentRow(m_nCurrentPlayListIndex);       // ui选中状态更新为该url
    // 保存更新后的播放列表
    savePlayList();
}

void Playlist::OnAddFileAndPlay(QString strFileName)
{
    bool bSupportMovie = strFileName.endsWith(".mkv", Qt::CaseInsensitive) ||
                         strFileName.endsWith(".rmvb", Qt::CaseInsensitive) ||
                         strFileName.endsWith(".mp4", Qt::CaseInsensitive) ||
                         strFileName.endsWith(".avi", Qt::CaseInsensitive) ||
                         strFileName.endsWith(".flv", Qt::CaseInsensitive) ||
                         strFileName.endsWith(".wmv", Qt::CaseInsensitive) ||
                         strFileName.endsWith(".3gp", Qt::CaseInsensitive);
    if (!bSupportMovie) {
        return;
    }
    QFileInfo fileInfo(strFileName);
    QList<QListWidgetItem *> listItem = ui->List->findItems(fileInfo.fileName(), Qt::MatchExactly);
    QListWidgetItem *pItem = nullptr;
    if (listItem.isEmpty()) {
        pItem = new QListWidgetItem(ui->List);
        pItem->setData(Qt::UserRole, QVariant(fileInfo.filePath()));  // 用户数据
        pItem->setText(fileInfo.fileName());  // 显示文本
        pItem->setToolTip(fileInfo.filePath());
        ui->List->addItem(pItem);
    } else {
        pItem = listItem.at(0);
    }
    on_List_itemDoubleClicked(pItem);
    savePlayList();
}

void Playlist::OnBackwardPlay()
{
    if (m_nCurrentPlayListIndex == 0) {
        m_nCurrentPlayListIndex = ui->List->count() - 1;
        on_List_itemDoubleClicked(ui->List->item(m_nCurrentPlayListIndex));
        ui->List->setCurrentRow(m_nCurrentPlayListIndex);
    } else {
        m_nCurrentPlayListIndex--;
        on_List_itemDoubleClicked(ui->List->item(m_nCurrentPlayListIndex));
        ui->List->setCurrentRow(m_nCurrentPlayListIndex);
    }
}

void Playlist::OnForwardPlay()
{
    if (m_nCurrentPlayListIndex == ui->List->count() - 1) {
        m_nCurrentPlayListIndex = 0;
        on_List_itemDoubleClicked(ui->List->item(m_nCurrentPlayListIndex));
        ui->List->setCurrentRow(m_nCurrentPlayListIndex);
    } else {
        m_nCurrentPlayListIndex++;
        on_List_itemDoubleClicked(ui->List->item(m_nCurrentPlayListIndex));
        ui->List->setCurrentRow(m_nCurrentPlayListIndex);
    }
}

void Playlist::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) {
        return;
    }
    for (QUrl url : urls) {
        QString strFileName = url.toLocalFile();
        OnAddFile(strFileName);
    }
}

void Playlist::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void Playlist::on_List_itemSelectionChanged()
{
    m_nCurrentPlayListIndex = ui->List->currentIndex().row();      // 获取选中后的新位置。
}

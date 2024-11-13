/*
 * @file 	playlist.h
 * @date 	2018/01/07 11:12
 *
 * @author 	itisyang
 * @Contact	itisyang@gmail.com
 *
 * @brief 	播放列表控件
 * @note
 */
#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <QWidget>
#include <QListWidgetItem>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>

namespace Ui {
class Playlist;
}
//实现一个播放列表
class Playlist : public QWidget
{
    Q_OBJECT

public:
    explicit Playlist(QWidget *parent = 0);
    ~Playlist();

	bool Init();

    ////播放列表状态管理
	/**
	 * @brief	获取播放列表状态
	 * 
	 * @return	true 显示 false 隐藏
	 * @note 	
	 */
    bool GetPlaylistStatus();
    int GetCurrentIndex();
    std::string GetCurrentUrl();
    // 获取前一个url并将前一个url设置为选中状态
    std::string GetPrevUrlAndSelect();
    // 获取下一个url并将下一个url设置为选中状态
    std::string GetNextUrlAndSelect();
public:
    ////添加文件到播放列表,并提供了直接播放的功能。
    void AddNetworkUrl(QString network_url);
	/**
	 * @brief	添加文件
	 * 
	 * @param	strFileName 文件完整路径
	 * @note 	
	 */
    void OnAddFile(QString strFileName);
    //添加并播放
    void OnAddFileAndPlay(QString strFileName);

    ////播放控制
    //上一集/下一集 文件读取
    void OnBackwardPlay();
    void OnForwardPlay();


    void OnRequestPlayCurrentFile();
    //dock 窗口的初始大小
    QSize sizeHint() const
    {
        return QSize(150, 900);
    }
protected:
    ////拖放事件处理
    /**
    * @brief	放下事件
    *
    * @param	event 事件指针
    * @note
    */
    void dropEvent(QDropEvent *event);
    /**
    * @brief	拖动事件
    *
    * @param	event 事件指针
    * @note
    */
    void dragEnterEvent(QDragEnterEvent *event);

signals:
    void SigUpdateUi();	//< 界面排布更新
    void SigPlay(std::string url); //< 播放文件

private:
    bool InitUi();
    //检测连接类内部的信号和槽
    bool ConnectSignalSlots();
    void savePlayList();
private slots:
    // 双击播放响应
	void on_List_itemDoubleClicked(QListWidgetItem *item);
    //单击选中
    void on_List_itemSelectionChanged();

private:
    Ui::Playlist *ui;

    int m_nCurrentPlayListIndex = 0;
};

#endif // PLAYLIST_H

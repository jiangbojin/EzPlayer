#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>

#include "easylogging++.h"
#include "globalhelper.h"

const QString PLAYER_CONFIG_BASEDIR = QDir::tempPath();

const QString PLAYER_CONFIG = "player_config.ini";

const QString APP_VERSION = "0.1.0";

GlobalHelper::GlobalHelper() {}

QString GlobalHelper::GetQssStr(QString strQssPath) {
    QString strQss;
    QFile FileQss(strQssPath);
    if (FileQss.open(QIODevice::ReadOnly)) {
        strQss = FileQss.readAll();
        FileQss.close();
    } else {
        LOG(ERROR) << "读取样式表失败" << strQssPath.toStdString();
    }
    return strQss;
}

void GlobalHelper::SetIcon(QPushButton* btn, int iconSize, QChar icon) {
    QFont font;
    font.setFamily("FontAwesome");
    font.setPointSize(iconSize);

    btn->setFont(font);
    btn->setText(icon);
}
//将播放列表保存到配置文件中
void GlobalHelper::SavePlaylist(QStringList& playList) {
    //QString strPlayerConfigFileName = QCoreApplication::applicationDirPath() + QDir::separator() + PLAYER_CONFIG;
    QString strPlayerConfigFileName = PLAYER_CONFIG_BASEDIR + QDir::separator() + PLAYER_CONFIG;
    //读写配置文件 C:/Users/jbj/AppData/Local/Temp\player_config.ini
    QSettings settings(strPlayerConfigFileName, QSettings::IniFormat);
    settings.beginWriteArray("playlist");
    //播放列表数据到配置文件中

    for (int i = 0; i < playList.size(); ++i) { /*
2\movie=D:/10.mp4
3\movie=D:/time.flv
    */
        settings.setArrayIndex(i);
        settings.setValue("movie", playList.at(i));
    }
    settings.endArray();
}

void GlobalHelper::GetPlaylist(QStringList& playList) {
    //QString strPlayerConfigFileName = QCoreApplication::applicationDirPath() + QDir::separator() + PLAYER_CONFIG;
    QString strPlayerConfigFileName = PLAYER_CONFIG_BASEDIR + QDir::separator() + PLAYER_CONFIG;
    QSettings settings(strPlayerConfigFileName, QSettings::IniFormat);

    int size = settings.beginReadArray("playlist");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        playList.append(settings.value("movie").toString());
    }
    settings.endArray();
}

void GlobalHelper::SavePlayVolume(double& nVolume) {
    QString strPlayerConfigFileName = PLAYER_CONFIG_BASEDIR + QDir::separator() + PLAYER_CONFIG;
    QSettings settings(strPlayerConfigFileName, QSettings::IniFormat);
    settings.setValue("volume/size", nVolume);
}

void GlobalHelper::GetPlayVolume(double& nVolume) {
    QString strPlayerConfigFileName = PLAYER_CONFIG_BASEDIR + QDir::separator() + PLAYER_CONFIG;
    QSettings settings(strPlayerConfigFileName, QSettings::IniFormat);
    QString str = settings.value("volume/size").toString();
    nVolume     = settings.value("volume/size", nVolume).toDouble();
}

QString GlobalHelper::GetAppVersion() {
    return APP_VERSION;
}

#ifndef DEEPSEEKCLIENT_H
#define DEEPSEEKCLIENT_H

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslSocket>
#include <QWidget>

namespace Ui {
class DeepSeekClient;
}

class DeepSeekClient : public QWidget {
    Q_OBJECT

   public:
    explicit DeepSeekClient(QWidget* parent = nullptr);
    ~DeepSeekClient();
    void sendRequest(const QString& prompt);

    void handleResponse(QNetworkReply* reply);

   signals:
    void responseReceived(const QString& response);

   private slots:
    void on_pushButton_clicked();

   private:
    QNetworkAccessManager* manager;
    QString api_key = "sk-4028d337980f4681b36e7b0a77c3b511";  // 替换为你的API Key
   private:
    Ui::DeepSeekClient* ui;
};

#endif  // DEEPSEEKCLIENT_H

#include "deepseekclient.h"
#include <QDebug>
#include "log/easylogging++.h"
#include "ui_deepseekclient.h"

DeepSeekClient::DeepSeekClient(QWidget* parent) : QWidget(parent), ui(new Ui::DeepSeekClient) {
    ui->setupUi(this);
    manager = new QNetworkAccessManager(this);

    // 设置样式表
    QString styleSheet = R"(
        QWidget {
            background-color: #f5f5f5;
            font-family: "Microsoft YaHei", Arial;
        }
        QTextEdit {
            background-color: white;
            border: 1px solid #ddd;
            border-radius: 8px;
            padding: 8px;
            font-size: 14px;
        }
        QLineEdit {
            background-color: white;
            border: 1px solid #ddd;
            border-radius: 8px;
            padding: 4px 12px;
            font-size: 14px;
        }
        QLineEdit:focus {
            border: 1px solid #4a90e2;
        }
        QPushButton {
            background-color: #4a90e2;
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #357abd;
        }
        QPushButton:pressed {
            background-color: #2a5f96;
        }
    )";
    this->setStyleSheet(styleSheet);

    // 检查SSL支持
    qDebug() << "SSL Support:" << QSslSocket::supportsSsl();
    qDebug() << "Build Version:" << QSslSocket::sslLibraryBuildVersionString();
    qDebug() << "Runtime Version:" << QSslSocket::sslLibraryVersionString();

    qDebug() << manager->supportedSchemes();
}

DeepSeekClient::~DeepSeekClient() {
    delete ui;
}

///
/// dpseek发送数据
/// \param prompt
///
void DeepSeekClient::sendRequest(const QString& prompt) {

    // 显示思考状态
    QString userInput = ui->dpseek_input_edit->text();
    ui->dpseek_output_edit->append(
        QString("<div style='margin: 8px 0;'><b style='color: #4a90e2;'>您:</b> %1</div>")
            .arg(userInput));
    ui->dpseek_output_edit->append(
        QString("<div style='margin: 8px 0; color: #666;'><i>AI正在思考中，请稍候...</i></div>"));

    // 禁用输入和发送按钮
    ui->dpseek_input_edit->setEnabled(false);
    ui->pushButton->setEnabled(false);

    QUrl url("https://api.deepseek.com/chat/completions");
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + api_key).toUtf8());

    QJsonObject body;
    body["model"]    = "deepseek-chat";
    body["messages"] = QJsonArray{QJsonObject{{"role", "user"}, {"content", prompt}}};

    QNetworkReply* reply = manager->post(request, QJsonDocument(body).toJson());

    // 处理SSL错误
    connect(reply, &QNetworkReply::sslErrors, this, [reply]() {
        qDebug() << "SSL Errors occurred!";
        reply->ignoreSslErrors();  // 测试阶段忽略错误
    });

    connect(reply, &QNetworkReply::finished, [=]() {
        handleResponse(reply);
        reply->deleteLater();
    });
}
///
/// 接收数据，回调处理
/// \param reply
///
void DeepSeekClient::handleResponse(QNetworkReply* reply) {
    // 重新启用输入和发送按钮
    ui->dpseek_input_edit->setEnabled(true);
    ui->pushButton->setEnabled(true);

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument doc   = QJsonDocument::fromJson(response);
        QJsonObject json    = doc.object();

        if (json.contains("choices")) {
            QString result =
                json["choices"].toArray()[0].toObject()["message"].toObject()["content"].toString();

            // 删除"正在思考"的提示
            QTextCursor cursor = ui->dpseek_output_edit->textCursor();
            cursor.movePosition(QTextCursor::End);
            cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
            cursor.deletePreviousChar();  // 删除多余的换行

            // 显示AI回复
            ui->dpseek_output_edit->append(
                QString("<div style='margin: 8px 0; background-color: #f8f9fa; padding: 8px; "
                        "border-radius: 4px;'><b style='color: #28a745;'>AI:</b> %1</div>")
                    .arg(result));

            // 清空输入框
            ui->dpseek_input_edit->clear();

            emit responseReceived(result);
        }
    } else {
        // 删除"正在思考"的提示
        QTextCursor cursor = ui->dpseek_output_edit->textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.deletePreviousChar();  // 删除多余的换行

        QString errorMessage =
            QString("<div style='color: #dc3545; margin: 8px 0;'><b>错误:</b> %1</div>")
                .arg(reply->errorString());
        ui->dpseek_output_edit->append(errorMessage);
    }
}
void DeepSeekClient::on_pushButton_clicked() {
    LOG(INFO) << "INPUT: " << ui->dpseek_input_edit->text().toStdString();
    sendRequest(ui->dpseek_input_edit->text());
}

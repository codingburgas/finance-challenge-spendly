#include "firebase_rest_helper.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>

FirebaseRestHelper::FirebaseRestHelper(QObject* parent)
    : QObject(parent), networkManager(new QNetworkAccessManager(this)) {
    connect(networkManager, &QNetworkAccessManager::finished, this, &FirebaseRestHelper::onNetworkReply);
}

void FirebaseRestHelper::signUp(const QString& email, const QString& password) {
    QUrl url(QString("https://identitytoolkit.googleapis.com/v1/accounts:signUp?key=%1").arg(apiKey));
    QJsonObject payload = {{"email", email}, {"password", password}, {"returnSecureToken", true}};

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    networkManager->post(request, QJsonDocument(payload).toJson());
}

void FirebaseRestHelper::signIn(const QString& email, const QString& password) {
    QUrl url(QString("https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=%1").arg(apiKey));
    QJsonObject payload = {{"email", email}, {"password", password}, {"returnSecureToken", true}};

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    networkManager->post(request, QJsonDocument(payload).toJson());
}

void FirebaseRestHelper::updateUserProfile(const QString& userId, const QString& username, int monthlyIncome) {
    QUrl url(QString("https://firestore.googleapis.com/v1/projects/spendly-3412a/databases/(default)/documents/users/%1").arg(userId));
    QJsonObject data;
    data["fields"] = QJsonObject{
        {"username", QJsonObject{{"stringValue", username}}},
        {"monthlyIncome", QJsonObject{{"integerValue", monthlyIncome}}}
    };

    QNetworkRequest request(url);
    request.setRawHeader("Content-Type", "application/json");
    networkManager->sendCustomRequest(request, "PATCH", QJsonDocument(data).toJson());
}

void FirebaseRestHelper::fetchUserProfile(const QString& userId) {
    QUrl url(QString("https://firestore.googleapis.com/v1/projects/spendly-3412a/databases/(default)/documents/users/%1").arg(userId));

    QNetworkRequest request(url);
    request.setRawHeader("Content-Type", "application/json");
    networkManager->get(request);
}

void FirebaseRestHelper::onNetworkReply(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        auto jsonResponse = QJsonDocument::fromJson(reply->readAll()).object();

        // Handle authentication response
        if (jsonResponse.contains("idToken")) {
            QString userId = jsonResponse.value("localId").toString();  // Get the user ID
            emit authenticationSuccess(userId);
        }
        // Handle profile response
        else if (jsonResponse.contains("fields") && jsonResponse["fields"].isObject()) {
            QJsonObject fields = jsonResponse["fields"].toObject();
            QString username;
            int monthlyIncome = 0;

            // Extract username
            if (fields.contains("username") && fields["username"].isObject()) {
                username = fields["username"].toObject().value("stringValue").toString();
            }

            // Extract monthlyIncome
            if (fields.contains("monthlyIncome") && fields["monthlyIncome"].isObject()) {
                QJsonObject monthlyIncomeObj = fields["monthlyIncome"].toObject();
                if (monthlyIncomeObj.contains("integerValue")) {
                    // Convert the string value to an integer
                    monthlyIncome = monthlyIncomeObj.value("integerValue").toString().toInt();
                }
            }

            emit userProfileFetched(username, monthlyIncome);
        }
    } else {
        emit authenticationFailed(reply->errorString());
    }
    reply->deleteLater();
}



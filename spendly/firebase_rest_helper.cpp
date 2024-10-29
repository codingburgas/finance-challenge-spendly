#include "firebase_rest_helper.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QDebug>
#include <QJsonArray>

FirebaseRestHelper::FirebaseRestHelper(QObject* parent)
    : QObject(parent), networkManager(new QNetworkAccessManager(this)) {
    connect(networkManager, &QNetworkAccessManager::finished, this, &FirebaseRestHelper::onNetworkReply);
}

void FirebaseRestHelper::signUp(const QString& email, const QString& password) {
    if (apiKey.isEmpty()) {
        emit authenticationFailed("API Key not set");
        return;
    }
    QUrl url(QString("https://identitytoolkit.googleapis.com/v1/accounts:signUp?key=%1").arg(apiKey));
    QJsonObject payload = {{"email", email}, {"password", password}, {"returnSecureToken", true}};

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    networkManager->post(request, QJsonDocument(payload).toJson());
}

void FirebaseRestHelper::signIn(const QString& email, const QString& password) {
    if (apiKey.isEmpty()) {
        emit authenticationFailed("API Key not set");
        return;
    }
    QUrl url(QString("https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=%1").arg(apiKey));
    QJsonObject payload = {{"email", email}, {"password", password}, {"returnSecureToken", true}};

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    networkManager->post(request, QJsonDocument(payload).toJson());
}

void FirebaseRestHelper::updateUserProfile(const QString& userId, const QString& username, double monthlyIncome) {
    if (userId.isEmpty()) {
        qWarning() << "User ID is empty";
        return;
    }
    QUrl url(QString("https://firestore.googleapis.com/v1/projects/spendly-3412a/databases/(default)/documents/users/%1").arg(userId));
    QJsonObject data;
    data["fields"] = QJsonObject{
        {"username", QJsonObject{{"stringValue", username}}},
        {"monthlyIncome", QJsonObject{{"doubleValue", QString::number(monthlyIncome, 'f', 2)}}}
    };

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + idToken.toUtf8()); // Add authentication token
    networkManager->sendCustomRequest(request, "PATCH", QJsonDocument(data).toJson());
}

void FirebaseRestHelper::fetchUserProfile(const QString& userId) {
    if (userId.isEmpty() || idToken.isEmpty()) {
        qWarning() << "User ID or ID Token is empty";
        return;
    }
    QUrl url(QString("https://firestore.googleapis.com/v1/projects/spendly-3412a/databases/(default)/documents/users/%1").arg(userId));

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + idToken.toUtf8());

    networkManager->get(request);
}

void FirebaseRestHelper::fetchUserReceipts(const QString& userId) {
    if (userId.isEmpty() || idToken.isEmpty()) {
        qWarning() << "User ID or ID Token is empty";
        return;
    }

    QUrl url(QString("https://firestore.googleapis.com/v1/projects/spendly-3412a/databases/(default)/documents/users-receipts/%1").arg(userId));

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + idToken.toUtf8());

    networkManager->get(request);
}


void FirebaseRestHelper::onNetworkReply(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        auto jsonResponse = QJsonDocument::fromJson(reply->readAll()).object();
        //qDebug() << "JSON Response:" << jsonResponse;

        // Check if the response is an authentication response
        if (jsonResponse.contains("idToken")) {
            QString userId = jsonResponse.value("localId").toString();
            idToken = jsonResponse.value("idToken").toString();
            emit authenticationSuccess(userId); // Emit only `userId`
        }

        // Check if the response is a user profile response
        if (jsonResponse.contains("fields") && jsonResponse["fields"].isObject()) {
            QJsonObject fields = jsonResponse["fields"].toObject();
            QString username;
            double monthlyIncome = 0.0;
            bool validProfile = false; // Flag to track if we have a valid profile

            // Extract username
            if (fields.contains("username") && fields["username"].isObject()) {
                username = fields["username"].toObject().value("stringValue").toString();
            }

            // Extract monthly income
            if (fields.contains("monthlyIncome") && fields["monthlyIncome"].isObject()) {
                monthlyIncome = fields["monthlyIncome"].toObject().value("doubleValue").toDouble(); // Directly convert to double
            }

            // Check if both username and monthlyIncome are valid
            if (!username.isEmpty() && monthlyIncome >= 0.0) {
                validProfile = true; // Set flag to true if profile is valid
            }

            // Emit the signal only if the profile is valid
            if (validProfile) {
                emit userProfileFetched(username, monthlyIncome);
            } else {
                qDebug() << "Invalid profile data, signal not emitted. Username:" << username << "Monthly Income:" << monthlyIncome;
            }
        }


        // Check if the response is a receipts response
        if (jsonResponse.contains("fields") && jsonResponse["fields"].isObject()) {
            QJsonObject fields = jsonResponse["fields"].toObject();
            QList<QVariantMap> receipts;

            // Iterate through the fields, which are presumably receipt identifiers
            for (const QString& key : fields.keys()) {
                QJsonObject receipt = fields[key].toObject();
                QJsonObject mapValue = receipt["mapValue"].toObject();

                QVariantMap receiptData;

                // Extract fields from the receipt
                if (mapValue.contains("fields")) {
                    QJsonObject receiptFields = mapValue["fields"].toObject();

                    // Extract date
                    if (receiptFields.contains("date") && receiptFields["date"].isObject()) {
                        QString isoDate = receiptFields["date"].toObject()["mapValue"].toObject()["fields"].toObject()["data"].toObject()["stringValue"].toString();
                        // Format the date to YYYY-MM-DD
                        QString formattedDate = isoDate.left(10); // Get first 10 characters (YYYY-MM-DD)
                        receiptData["date"] = formattedDate;
                    }

                    // Extract totalAmount
                    if (receiptFields.contains("totalAmount") && receiptFields["totalAmount"].isObject()) {
                        receiptData["totalAmount"] = receiptFields["totalAmount"].toObject()["mapValue"].toObject()["fields"].toObject()["data"].toObject()["doubleValue"].toDouble();
                    }
                }

                // Only add to receipts if data is valid
                if (!receiptData["date"].toString().isEmpty() || receiptData["totalAmount"].toDouble() != 0.0) {
                    receipts.append(receiptData);
                }
            }

            // Emit the signal only if receipts is not empty
            if (!receipts.isEmpty()) {
                emit userReceiptsFetched(receipts); // Emit the signal with the list of receipts
            } else {
                qDebug() << "No valid receipts found, signal not emitted.";
            }
        }


    } else {
        qWarning() << "Network Error:" << reply->errorString();
        emit authenticationFailed(reply->errorString());
    }
    reply->deleteLater();
}



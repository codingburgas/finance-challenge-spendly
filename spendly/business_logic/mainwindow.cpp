#include "mainwindow.h"
#include <QWebChannel>
#include <QTimer>
#include <QOperatingSystemVersion>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>

#ifdef Q_OS_MAC
    int timerDuration = 0; 
#else
    int timerDuration = 400;
#endif
// Function to export all reciepts in a csv file
void exportReceiptsToCsv(const QList<QMap<QString, QVariant>>& receipts, const QString& filePath) {
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);


        out << "Date,Amount,Description\n";

        for (const auto& receipt : receipts) {
            QString row = QString("%1,%2,%3\n")
            .arg(receipt["date"].toString())
                .arg(receipt["amount"].toDouble())
                .arg(receipt["description"].toString());
            out << row;
        }

        file.close();
        qDebug() << "Receipts exported to" << filePath;
    } else {
        qDebug() << "Failed to open file" << filePath;
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), view(new QWebEngineView(this)), firebaseHelper(new FirebaseRestHelper(this)) {

    // Set up QWebChannel
    QWebChannel* channel = new QWebChannel(this);
    channel->registerObject("backend", this);
    view->page()->setWebChannel(channel);

    // Load initial HTML page
    view->load(QUrl("qrc:/html/index.html"));
    view->resize(410, 680);
    this->setFixedSize(view->size());
    setCentralWidget(view);

    // Connect authenticationSuccess signal to handle login and load home.html
    connect(firebaseHelper, &FirebaseRestHelper::authenticationSuccess, this, [this](const QString& userId) {
        runJavaScript(QString("localStorage.setItem('userId', '%1');").arg(userId));
        firebaseHelper->fetchUserProfile(userId);
        // firebaseHelper->fetchUserReceipts(userId);

        runJavaScript("window.location.href = 'qrc:/html/home.html';");
    });

    // Connect userProfileFetched signal with a delayed call to populateUserProfile
    connect(firebaseHelper, &FirebaseRestHelper::userProfileFetched, this, [this](const QString& username, double monthlyIncome) {
        qDebug() << "Calling runJavaScript with: " << username << ", " << monthlyIncome;


        QTimer::singleShot(timerDuration, this, [this, username, monthlyIncome]() {
            runJavaScript(QString("populateUserProfile('%1', %2);").arg(username).arg(monthlyIncome));
        });
    });

    connect(firebaseHelper, &FirebaseRestHelper::userReceiptsFetched, this, [this](const QList<QVariantMap>& receipts) {
        qDebug() << "Fetched receipts count:" << receipts.size();

        QString jsArray = "[";
        double maxAmount = 0.0;
        double latestAmount = 0.0;
        QString latestDate;

        if (!receipts.isEmpty()) {
            latestAmount = receipts.first()["totalAmount"].toDouble();
            latestDate = receipts.first()["date"].toString();
        }

        for (const auto& receipt : receipts) {
            double amount = receipt["totalAmount"].toDouble();
            maxAmount = std::max(maxAmount, amount);

            jsArray += QString("{date: '%1', amount: '%2', description: 'Receipt'}")
                           .arg(receipt["date"].toString())
                           .arg(amount);
            if (&receipt != &receipts.last()) {
                jsArray += ",";
            }
        }
        jsArray += "]";

        qDebug() << "JavaScript Array:" << jsArray;

        // Pass maxAmount, latestAmount, and latestDate to JavaScript along with the history data
        QTimer::singleShot(timerDuration, this, [this, jsArray, maxAmount, latestAmount, latestDate]() {
            runJavaScript(QString("historyData = %1; biggestPurchaseAmount = %2; latestPurchaseAmount = %3; latestPurchaseDate = '%4'; loadHistory();")
                              .arg(jsArray)
                              .arg(maxAmount)
                              .arg(latestAmount)
                              .arg(latestDate));
        });
    });



    // Connect the loadFinished signal to ensure the page is fully loaded before any JS calls
    connect(view, &QWebEngineView::loadFinished, this, [](bool ok) {
        if (ok) {
            qDebug() << "Page has loaded successfully.";
        } else {
            qDebug() << "Failed to load page.";
        }
    });
}

MainWindow::~MainWindow() {
    delete view;
    delete firebaseHelper;
}
// Log in and Sign up function
void MainWindow::handleAuthenticationRequest(const QString& type, const QString& email, const QString& password) {
    if (type == "signIn") {
        firebaseHelper->signIn(email, password);
    } else if (type == "signUp") {
        firebaseHelper->signUp(email, password);
    }
}

void MainWindow::handleProfileUpdate(const QString& userId, const QString& username, double monthlyIncome) {
    firebaseHelper->updateUserProfile(userId, username, monthlyIncome);
    //firebaseHelper->fetchUserReceipts(userId);
}

void MainWindow::runJavaScript(const QString& script) {
    view->page()->runJavaScript(script);
}
// Function that loads reciepts
void MainWindow::loadHistory(const QString& userId) {
    QTimer::singleShot(timerDuration, this, [this, userId]() {
        firebaseHelper->fetchUserReceipts(userId);
    });
}
// Function that converts reciepts values to strings
void MainWindow::exportReceipts(const QJsonArray& receiptsJson) {
    QList<QMap<QString, QVariant>> receipts;

    for (const QJsonValue& value : receiptsJson) {
        QJsonObject obj = value.toObject();
        QMap<QString, QVariant> receipt;

        receipt["date"] = obj["date"].toString();
        receipt["amount"] = obj["amount"].toString().toDouble();
        receipt["description"] = obj["description"].toString();

        receipts.append(receipt);
    }

    QString filePath = QCoreApplication::applicationDirPath() + "/../../../../../receipts.csv";
    exportReceiptsToCsv(receipts, filePath);
    runJavaScript("showExportSuccessMessage();");
}


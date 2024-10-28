#include "mainwindow.h"
#include <QWebChannel>
#include <QTimer>
#include <QOperatingSystemVersion>

int timerDuration = 100; // Default for macOS
#ifdef Q_OS_WIN
    timerDuration = 400; // Override for Windows
#endif

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

        // Only fetch receipts if you're going to the history screen after login
        // Comment out the next line to stop fetching receipts during login
        // firebaseHelper->fetchUserReceipts(userId);

        runJavaScript("window.location.href = 'qrc:/html/home.html';");
    });

    // Connect userProfileFetched signal with a delayed call to populateUserProfile
    connect(firebaseHelper, &FirebaseRestHelper::userProfileFetched, this, [this](const QString& username, double monthlyIncome) {
        qDebug() << "Calling runJavaScript with: " << username << ", " << monthlyIncome;

        // Create a QTimer to introduce a delay before calling the JavaScript function
        QTimer::singleShot(timerDuration, this, [this, username, monthlyIncome]() {
            runJavaScript(QString("populateUserProfile('%1', %2);").arg(username).arg(monthlyIncome));
        });
    });

    // Connect userReceiptsFetched signal to handle the receipt data
    connect(firebaseHelper, &FirebaseRestHelper::userReceiptsFetched, this, [this](const QList<QVariantMap>& receipts) {
        // Log the number of receipts fetched
        qDebug() << "Fetched receipts count:" << receipts.size();

        QString jsArray = "[";
        for (const auto& receipt : receipts) {
            jsArray += QString("{date: '%1', amount: '%2', description: 'Receipt'}")
            .arg(receipt["date"].toString())
                .arg(receipt["totalAmount"].toDouble());
            if (&receipt != &receipts.last()) {
                jsArray += ",";
            }
        }
        jsArray += "]";

        // Log the constructed JavaScript array
        qDebug() << "JavaScript Array:" << jsArray;

        // Update the historyData variable and call loadHistory
        runJavaScript(QString("historyData = %1; loadHistory();").arg(jsArray));
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

void MainWindow::handleAuthenticationRequest(const QString& type, const QString& email, const QString& password) {
    if (type == "signIn") {
        firebaseHelper->signIn(email, password);
    } else if (type == "signUp") {
        firebaseHelper->signUp(email, password);
    }
}

void MainWindow::handleProfileUpdate(const QString& userId, const QString& username, int monthlyIncome) {
    firebaseHelper->updateUserProfile(userId, username, monthlyIncome);
}

void MainWindow::runJavaScript(const QString& script) {
    view->page()->runJavaScript(script);
}

// New method to load receipts for history
void MainWindow::loadHistory(const QString& userId) {
    firebaseHelper->fetchUserReceipts(userId);
}

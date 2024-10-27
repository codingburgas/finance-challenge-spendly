#include "mainwindow.h"
#include <QWebChannel>
#include <QTimer>

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
        runJavaScript("window.location.href = 'qrc:/html/home.html';");
    });

    // Connect userProfileFetched signal with a delayed call to populateUserProfile
    connect(firebaseHelper, &FirebaseRestHelper::userProfileFetched, this, [this](const QString& username, double monthlyIncome) {
        qDebug() << "Calling runJavaScript with: " << username << ", " << monthlyIncome;

        // Create a QTimer to introduce a delay before calling the JavaScript function
        QTimer::singleShot(400, this, [this, username, monthlyIncome]() {
            runJavaScript(QString("populateUserProfile('%1', %2);").arg(username).arg(monthlyIncome));
        });
    });

    // Connect the loadFinished signal to ensure the page is fully loaded before any JS calls
    connect(view, &QWebEngineView::loadFinished, this, [this](bool ok) {
        if (ok) {
            qDebug() << "home.html has loaded successfully. Ready to call JavaScript.";
        } else {
            qDebug() << "Failed to load home.html.";
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

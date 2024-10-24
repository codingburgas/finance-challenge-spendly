#include "mainwindow.h"
#include <QWebChannel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), view(new QWebEngineView(this)), firebaseHelper(new FirebaseRestHelper(this)) {
    QWebChannel* channel = new QWebChannel(this);
    channel->registerObject("backend", this);
    view->page()->setWebChannel(channel);
    view->load(QUrl("qrc:/html/index.html"));
    view->resize(410, 680);
    this->setFixedSize(view->size());
    setCentralWidget(view);

    connect(firebaseHelper, &FirebaseRestHelper::authenticationSuccess, this, [this](const QString& userId) {
        runJavaScript(QString("localStorage.setItem('userId', '%1');").arg(userId));
        firebaseHelper->fetchUserProfile(userId);
        runJavaScript("window.location.href = 'home.html';"); // Redirect to home.html
    });
    connect(firebaseHelper, &FirebaseRestHelper::authenticationFailed, this, [this](const QString& error) {
        runJavaScript(QString("showStatusMessage('Authentication failed: %1', false);").arg(error));
    });
    connect(firebaseHelper, &FirebaseRestHelper::userProfileFetched, this, [this](const QString& username, double monthlyIncome) {
        qDebug() << "Calling runJavaScript with: " << username << ", " << monthlyIncome;
        runJavaScript(QString("populateUserProfile('%1', %2);").arg(username).arg(monthlyIncome));
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

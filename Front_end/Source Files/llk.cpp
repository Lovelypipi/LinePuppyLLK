#include "llk.h"
#include "logindialog.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QLibraryInfo>
#include <QtCore/QLocale>

namespace{
QString ResolveTranslationPath(){
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString appTranslations = QDir(appDir).filePath("translations");
    if(QFileInfo::exists(appTranslations)){
        return appTranslations;
    }
    if(QFileInfo::exists(appDir)){
        return appDir;
    }
    return QLibraryInfo::path(QLibraryInfo::TranslationsPath);
}
}

LLKApp::LLKApp(int argc, char* argv[])
    : QApplication(argc, argv), m_pMainDlg(NULL){
    QCoreApplication::setOrganizationName("LLK");
    QCoreApplication::setApplicationName("LinePuppyLLK");

    QLocale::setDefault(QLocale(QLocale::Chinese, QLocale::China));

    const QString translationPath = ResolveTranslationPath();
    if(m_qtBaseTranslator.load("qtbase_zh_CN", translationPath)){
        installTranslator(&m_qtBaseTranslator);
    }
    if(m_qtTranslator.load("qt_zh_CN", translationPath)){
        installTranslator(&m_qtTranslator);
    }

    m_pMainDlg = new LLKDialog;
}

int LLKApp::Run(){
    if(!m_pMainDlg){
        m_pMainDlg = new LLKDialog;
    }
    m_pMainDlg->show();

    LoginDialog loginDialog(m_pMainDlg);
    if(loginDialog.exec() != QDialog::Accepted){
        m_pMainDlg->close();
        return 0;
    }

    const qint64 userId = loginDialog.loggedInUserId();
    const QString nickname = loginDialog.loggedInNickname().trimmed();
    m_pMainDlg->SetCurrentUser(userId, nickname);
    if(!nickname.isEmpty()){
        m_pMainDlg->setWindowTitle(QString("欢乐连连看 - 玩家：%1").arg(nickname));
    }
    return exec();
}

int main(int argc, char* argv[]){
    LLKApp app(argc, argv);
    return app.Run();
}

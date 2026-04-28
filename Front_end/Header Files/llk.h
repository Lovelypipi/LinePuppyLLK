#ifndef LLK_H
#define LLK_H

#include "stdafx.h"
#include "llkdialog.h"
#include <QtCore/QTranslator>

class LLKApp : public QApplication{
public:
    LLKApp(int argc, char* argv[]);
    int Run();
private:
    LLKDialog* m_pMainDlg;
    QTranslator m_qtBaseTranslator;
    QTranslator m_qtTranslator;
};

#endif
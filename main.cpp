#include "mainwindow.h"
#include "myapp.h"
#include <QApplication>

extern QVector<QString> filelist;
extern QWidgetList wdlist;
extern QString fileName;
MainWindow* mw_one;

int main(int argc, char* argv[])
{
#ifdef Q_OS_LINUX
    qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

#endif

    //QApplication a(argc, argv);
    MyApplication* a = new MyApplication(argc, argv);

    QTranslator translator0; //注意：在外面定义它
    QTranslator translator1;
    QTranslator translator2;
    QTextCodec* codec = QTextCodec::codecForName("System");
    QTextCodec::setCodecForLocale(codec);
    QLocale locale;
    if (locale.language() == QLocale::Chinese) {

        if (translator0.load(":/tr/qt_zh_CN.qm")) {
            qApp->installTranslator(&translator0);
        }

        if (translator1.load(":/tr/widgets_zh_cn.qm")) {
            qApp->installTranslator(&translator1);
        }

        if (translator2.load(":/tr/qscintilla_cn.qm")) {
            qApp->installTranslator(&translator2);
        }
    }

    /*注册线程间信号槽传递自定义数据类型*/
    qRegisterMetaType<QList<QPersistentModelIndex>>("QList<QPersistentModelIndex>");
    qRegisterMetaType<QList<QPersistentModelIndex>>("QList<QPersistentModelIndex>&");

    qRegisterMetaType<QAbstractItemModel::LayoutChangeHint>("QAbstractItemModel::LayoutChangeHint");
    qRegisterMetaType<QAbstractItemModel::LayoutChangeHint>("QAbstractItemModel::LayoutChangeHint&");

#ifdef Q_OS_WIN32

    fileName = QString::fromLocal8Bit(argv[1]); //解决乱码

    QFont f;
    f.setFamily("Microsoft YaHei UI");
    a->setFont(f);

#endif

#ifdef Q_OS_MAC

#endif

    if (!fileName.isEmpty()) {
        a->new_win();
    }

    else {

        mw_one = new MainWindow();
        mw_one->show();
    }

    return a->exec();
}

#include "mainwindow.h"
#include "myapp.h"
#include <QApplication>


extern QVector<QString> filelist;
extern QWidgetList wdlist;
extern QString fileName;
MainWindow * mw_one;

int main(int argc, char *argv[])
{

    //QApplication a(argc, argv);
    MyApplication *a = new MyApplication(argc, argv);

    /*注册线程间信号槽传递自定义数据类型*/
    qRegisterMetaType<QList<QPersistentModelIndex>>("QList<QPersistentModelIndex>");
    qRegisterMetaType<QList<QPersistentModelIndex>>("QList<QPersistentModelIndex>&");

    qRegisterMetaType<QAbstractItemModel::LayoutChangeHint>("QAbstractItemModel::LayoutChangeHint");
    qRegisterMetaType<QAbstractItemModel::LayoutChangeHint>("QAbstractItemModel::LayoutChangeHint&");

#ifdef Q_OS_WIN32

   fileName = QString::fromLocal8Bit(argv[1]);//解决乱码

#endif

#ifdef Q_OS_LINUX

#endif

#ifdef Q_OS_MAC

#endif

    if(!fileName.isEmpty())
    {
        a->new_win();
    }

    else
    {

        mw_one = new MainWindow();
        mw_one->show();

    }

    return a->exec();

}


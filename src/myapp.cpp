#include "myapp.h"

#include "mainwindow.h"

extern QString fileName, CurVersion;
extern QVector<QString> filelist;
extern QWidgetList wdlist;
extern MainWindow* mw_one;

bool MyApplication::event(QEvent* event) {
  if (event->type() == QEvent::FileOpen) {
    QFileOpenEvent* openEvent = static_cast<QFileOpenEvent*>(event);
    fileName = openEvent->file();

    if (mw_one->isActiveWindow()) {
    }

    mw_one->loadFile(mw_one->openFile(fileName), -1, -1);
  }

  return QApplication::event(event);
}

void MyApplication::new_win() {
  if (!fileName.isEmpty()) {
    bool newfile = true;
    for (int i = 0; i < filelist.count(); i++) {
      if (filelist.at(i) == fileName) {
        newfile = false;
        setActiveWindow(wdlist.at(i));
        wdlist.at(i)->raise();

        break;
      }
    }

    if (newfile) {
      MainWindow* mw = new MainWindow();
      mw_one = mw;
      mw->loadFile(mw->openFile(fileName), -1, -1);
      mw->show();
      filelist.push_back(fileName);

      wdlist.push_back(mw);
    }
  }
}

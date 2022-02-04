#include "mytabwidget.h"

#include <QApplication>
#include <QCursor>
#include <QDebug>
#include <QDir>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QSettings>

#include "MyTabBar.h"
#include "MyTabPage.h"
#include "MyTabPopup.h"
#include "filesystemwatcher.h"
#include "mainwindow.h"

extern MainWindow* mw_one;
extern QString dragFileName;
extern int rowDrag;
extern int colDrag;
extern int vs;
extern int hs;

extern QVector<QString> openFileList;

MyTabWidget::MyTabWidget(QWidget* parent) : QTabWidget(parent) {
  setAttribute(Qt::WA_StyledBackground);

  initTabBar();
}

int MyTabWidget::appendNormalPage(QWidget* page) {
  if (!page) return -1;
  //设置为调用close时释放
  page->setAttribute(Qt::WA_DeleteOnClose);
  //最后是添加到stackedWidget去了
  const int index = addTab(page, page->windowTitle());
  //切换为当前新增页
  setCurrentIndex(index);
  return index;
}

void MyTabWidget::removeNormalPage(QWidget* page) {
  if (!page) return;
  removeNormalIndex(indexOf(page));
}

void MyTabWidget::takeNormalPage(QWidget* page) {
  if (!page) return;
  removeTab(indexOf(page));
}

void MyTabWidget::removeNormalIndex(int index) {
  if (indexValid(index)) {
    QWidget* page = this->widget(index);
    //判断是否为固定不删除的
    if (!page || fixedPage.contains(page)) return;
    // removeTab只是从tabbar移除了，并没有释放
    removeTab(index);
    //可以自己调用delete，或者设置tab页为WA_DeleteOnClose,关闭时释放

    page->close();
  }
}

void MyTabWidget::removeCurrentPage() { removeNormalIndex(currentIndex()); }

void MyTabWidget::clearNormalPage() {
  const int tab_count = this->count();
  //从后往前删，这样index位置就是固定的
  for (int index = tab_count - 1; index >= 0; index--) {
    removeNormalIndex(index);
  }
}

void MyTabWidget::clearAllPage() {
  //全部移除
  while (this->count() > 0) {
    QWidget* page = this->widget(0);
    removeTab(0);
    if (page) page->close();
  }
  fixedPage.clear();
}

void MyTabWidget::appendFixedPage(QWidget* page) {
  if (!page) return;
  appendNormalPage(page);
  setFixedPage(page);
}

void MyTabWidget::setFixedPage(QWidget* page) {
  if (!page) return;
  setFixedIndex(indexOf(page));
}

void MyTabWidget::setFixedIndex(int index) {
  if (indexValid(index)) {
    QWidget* page = this->widget(index);
    if (page && !fixedPage.contains(page)) {
      fixedPage.push_back(page);
      //不显示关闭按钮，替换为nullptr
      tabBar()->setTabButton(index, QTabBar::RightSide, nullptr);
    }
  }
}

bool MyTabWidget::indexValid(int index) const {
  if (index < 0 || index >= this->count()) return false;
  return true;
}

bool MyTabWidget::pageValid(QWidget* page) const {
  if (!page) return false;
  return indexValid(indexOf(page));
}

void MyTabWidget::showEvent(QShowEvent* event) {
  QTabWidget::showEvent(event);
  //初始化时把已有的设置为close释放
  for (int index = 0; index < this->count(); index++) {
    QWidget* page = this->widget(index);
    if (page) page->setAttribute(Qt::WA_DeleteOnClose);
  }
}

void MyTabWidget::initTabBar() {
  MyTabBar* bar = new MyTabBar(this);
  // setTabBar是protected成员函数，要使用就得继承
  setTabBar(bar);

  //点击页签上的关闭按钮时，触发信号(屏蔽，用tabBar自身的移除功能)
  // connect(bar, &QTabBar::tabCloseRequested, this,
  // &MyTabWidget::removeNormalIndex);

  //拖拽到外部-还未释放鼠标
  connect(bar, &MyTabBar::beginDragOut, this, [this, bar](int index) {
    if (!indexValid(index)) return;
    QWidget* drag_tab = this->widget(index);
    //固定tab 或者 只有一个tab 不让拖出
    if (!drag_tab || fixedPage.contains(drag_tab) || bar->count() == 1) return;

    QPixmap pixmap(drag_tab->size() + QSize(2, 31));
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    if (painter.isActive()) {
      QRect title_rect{0, 0, pixmap.width(), 30};
      painter.fillRect(title_rect, Qt::white);
      painter.drawText(title_rect, Qt::AlignLeft | Qt::AlignVCenter,
                       "  " + drag_tab->windowTitle());
      painter.drawRect(pixmap.rect().adjusted(0, 0, -1, -1));
    }
    painter.end();
    drag_tab->render(&pixmap, QPoint(1, 30));

    QMimeData* mime = new QMimeData;
    QDrag* drag = new QDrag(bar);
    drag->setMimeData(mime);
    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(10, 0));

    //鼠标弹起后drag就释放了，这时候去判断是否拖拽到了外部
    connect(drag, &QDrag::destroyed, this, [=] {
      QPoint bar_point = bar->mapFromGlobal(QCursor::pos());
      //不在范围，拖出
      if (!bar->contentsRect().contains(bar_point)) {
        popPage(drag_tab);
      }
    });

    drag->exec(Qt::MoveAction);
  });
}

void MyTabWidget::popPage(QWidget* page) {
  takeNormalPage(page);
  //这里套一个自定义标题栏的窗口给page
  MyTabPopup* pop = new MyTabPopup(this);
  pop->setAttribute(Qt::WA_DeleteOnClose);
  pop->setContentWidget(page);
  pop->setWindowTitle(page->windowTitle());
  pop->resize(page->size());
  //拖出来的位置有点偏移
  pop->move(QCursor::pos() - QPoint(10, 10));

  //判断独立窗口是否拖回tab
  connect(pop, &MyTabPopup::dragRelease, this, [=](const QPoint& pos) {
    const QPoint bar_pos = tabBar()->mapFromGlobal(pos);
    //如果又拖回了tabbar范围内，就把widget取出来放回tab
    if (tabBar()->contentsRect().contains(bar_pos)) {
      QWidget* content = pop->getContentWidget();
      this->appendNormalPage(content);
      pop->disconnect();

      pop->close();
    }
  });

  QSettings Reg(mw_one->strIniFile, QSettings::IniFormat);
  Reg.setValue("restore", true);
  Reg.setValue("count", 1);

  Reg.setValue(QString::number(0) + "/" + "file", dragFileName);
  Reg.setValue(QString::number(0) + "/" + "row", rowDrag);
  Reg.setValue(QString::number(0) + "/" + "col", colDrag);
  Reg.setValue(QString::number(0) + "/" + "vs", vs);
  Reg.setValue(QString::number(0) + "/" + "hs", hs);

  // file.close();

  FileSystemWatcher::removeWatchPath(dragFileName);
  for (int i = 0; i < openFileList.count(); i++) {
    if (dragFileName == openFileList.at(i)) {
      openFileList.remove(i);
      break;
    }
  }

  mw_one->on_NewWindow();

  mw_one->on_tabWidget_textEdit_tabBarClicked(this->currentIndex());
}

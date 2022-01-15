#include "dlgscrollbox.h"

#include "mainwindow.h"
#include "ui_dlgscrollbox.h"
#include "ui_mainwindow.h"
extern MainWindow* mw_one;
extern miniDialog* miniDlg;

dlgScrollBox::dlgScrollBox(QWidget* parent)
    : QDialog(parent), ui(new Ui::dlgScrollBox) {
  ui->setupUi(this);

  // 最顶层，在任何APP之上
  // this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint |
  //                     Qt::FramelessWindowHint);
  this->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint |
                       Qt::Tool | Qt::FramelessWindowHint);

  this->setStyleSheet(
      QString("border-width: 1px;"
              "border-style: solid;"
              "border-radius:2px;"
              "border-color: rgb(25,25,255);"
              "background-color:rgba(0,0,255,20);"));
}

dlgScrollBox::~dlgScrollBox() { delete ui; }

void dlgScrollBox::mouseMoveEvent(QMouseEvent* e) {
  miniDlg->close();

  if (isDrag & (e->buttons() & Qt::LeftButton)) {
    // move(e->globalPos() - m_position);
    move(e->pos() - m_position);
    e->accept();
  }

  int x, y, w, h;
  x = mw_one->x() + mw_one->width() - mw_one->miniEdit->width() - 2;
  y = e->globalY();
  w = mw_one->miniEdit->width();
  h = s_box_h;

  int y0, y1;
  y0 = mw_one->y() + mw_one->ui->tabWidget_textEdit->tabBar()->height() +
       +this->height();
  y1 = y0 + mw_one->miniEdit->height() - this->height();

  int thisP = 0;

  if (y <= y0) y = y0;
  if (y >= y1) {
    y = y1;
    thisP = mw_one->miniEdit->height();
  } else
    thisP = y - y0;

  qDebug() << y - y0;

  this->setGeometry(x, y, w, h);

  int t = mw_one->miniEdit->height();
  int m = mw_one->miniEdit->verticalScrollBar()->maximum();
  double b = (double)(thisP) / (double)t;
  int p = b * m;

  mw_one->miniEdit->verticalScrollBar()->setSliderPosition(p);
}

void dlgScrollBox::mousePressEvent(QMouseEvent* e) {
  if (e->button() == Qt::LeftButton) {
    isDrag = true;
    m_position = e->globalPos() - this->pos();
    e->accept();
  }
}

void dlgScrollBox::mouseReleaseEvent(QMouseEvent*) { isDrag = false; }

void dlgScrollBox::init_ScrollBox() {
  int x, y, w, h;
  x = mw_one->x() + mw_one->width() - mw_one->miniEdit->width() - 2;

  w = mw_one->miniEdit->width();
  h = s_box_h;

  int p0;
  int h0 = mw_one->miniEdit->height();
  int h1 = mw_one->miniEdit->verticalScrollBar()->maximum();
  int p1 = mw_one->miniEdit->verticalScrollBar()->sliderPosition();
  double b = (double)(p1) / (double)(h1);
  p0 = h0 * b;

  int y0, y1;
  y0 = mw_one->y() + mw_one->ui->tabWidget_textEdit->tabBar()->height();
  y1 = y0 + mw_one->miniEdit->height() - this->height();

  if (p0 == 0) y = y0 + this->height();

  if (p0 > 0 & p0 < this->height()) y = y0 + p0 + this->height();

  if (p0 > this->height()) y = p0 + y0;

  qDebug() << p0 << y << y1;

  if (p0 < 0) {
    close();
    return;
  }

  this->setGeometry(x, y, w, h);
  this->show();
}

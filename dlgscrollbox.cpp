#include "dlgscrollbox.h"

#include "mainwindow.h"
#include "ui_dlgscrollbox.h"
#include "ui_mainwindow.h"
extern MainWindow* mw_one;

dlgScrollBox::dlgScrollBox(QWidget* parent)
    : QDialog(parent), ui(new Ui::dlgScrollBox) {
  ui->setupUi(this);

  this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint |
                       Qt::FramelessWindowHint);

  this->setStyleSheet(
      QString("border-width: 1px;"
              "border-style: solid;"
              "border-radius:2px;"
              "border-color: rgb(25,25,255);"
              "background-color:rgba(0,0,255,20);"));
}

dlgScrollBox::~dlgScrollBox() { delete ui; }

void dlgScrollBox::mouseMoveEvent(QMouseEvent* e) {
  if (isDrag & (e->buttons() & Qt::LeftButton)) {
    move(e->globalPos() - m_position);
    e->accept();
  }

  int x, y, w, h;
  x = mw_one->x() + mw_one->ui->tabWidget_misc->width() +
      mw_one->textEdit->width() + 8;
  y = e->globalY();
  QPoint point;
  point.setX(mw_one->textEdit->x());
  point.setY(mw_one->textEdit->y());

  w = 70;
  h = 30;

  int h0 = mw_one->textEdit->height();
  if (y >= h0) y = h0;

  this->setGeometry(x, y, w, h);

  qDebug() << y << mapFromGlobal(point) << mw_one->textEdit->y() << h0;
}

void dlgScrollBox::mousePressEvent(QMouseEvent* e) {
  if (e->button() == Qt::LeftButton) {
    isDrag = true;
    m_position = e->globalPos() - this->pos();
    e->accept();
  }
}

void dlgScrollBox::mouseReleaseEvent(QMouseEvent*) { isDrag = false; }

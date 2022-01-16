#include "dlgscrollbox.h"

#include "mainwindow.h"
#include "ui_dlgscrollbox.h"
#include "ui_mainwindow.h"
extern MainWindow *mw_one;
extern miniDialog *miniDlg;

dlgScrollBox::dlgScrollBox(QWidget *parent) : QDialog(parent), ui(new Ui::dlgScrollBox)
{
    ui->setupUi(this);

    // MainWindow *mw_one1 = qobject_cast<MainWindow *>(parent);

    // 最顶层，在任何APP之上
    // this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint |
    //                     Qt::FramelessWindowHint);

    // this->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint |
    //                     Qt::Tool | Qt::FramelessWindowHint);

    this->setAutoFillBackground(true);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Background, Qt::blue);
    this->setPalette(palette);
    setWindowOpacity(0.3);

    /*setStyleSheet(QString("border-width: 1px;"
"border-style: solid;"
"border-radius:2px;"
"border-color: rgb(25,25,255);"
"background-color:rgba(0,0,255,20);"));*/
}

dlgScrollBox::~dlgScrollBox() { delete ui; }

void dlgScrollBox::mouseMoveEvent(QMouseEvent *e)
{
    y0 = mw_one->y()
         + (mw_one->height() - mw_one->ui->statusbar->height() - mw_one->miniEdit->height())
         + this->height();
    y1 = y0 + mw_one->miniEdit->height() - this->height();

    x = mw_one->x() + (mw_one->width() - mw_one->miniEdit->width() - 4);

    w = mw_one->miniEdit->width();
    h = s_box_h;

    if (isDrag & (e->buttons() & Qt::LeftButton)) {
        // move(e->globalPos() - m_position);
        QPoint pMove(x, e->globalY());
        move(pMove - m_position);

        e->accept();
    }

    int t = mw_one->miniEdit->height() - this->height();
    unsigned long max = mw_one->miniEdit->verticalScrollBar()->maximum();
    my = this->y();

    if (my <= y0) {
        my = y0;
        isDrag = false;
    }

    if (my >= y1) {
        my = y1;
        isDrag = false;
    }

    thisP = my - y0;
    double b = (double) (thisP) / (double) t;
    unsigned long p = b * max;

    mw_one->miniEdit->verticalScrollBar()->setSliderPosition(p);

    this->setGeometry(x, my, w, h);
}

void dlgScrollBox::mousePressEvent(QMouseEvent *e)
{
    y0 = mw_one->y()
         + (mw_one->height() - mw_one->ui->statusbar->height() - mw_one->miniEdit->height())
         + this->height();
    y1 = y0 + mw_one->miniEdit->height() - this->height();

    my = 0;

    if (e->button() == Qt::LeftButton) {
        isDrag = true;
        x = mw_one->x() + (mw_one->width() - mw_one->miniEdit->width() - 4);
        QPoint p1(x, e->globalY());
        QPoint p0(x, this->pos().y());
        // m_position = e->globalPos() - this->pos();
        m_position = p1 - p0;
        e->accept();
    }
}

void dlgScrollBox::mouseReleaseEvent(QMouseEvent *)
{
    isDrag = false;

    if (my == y0)
        this->setGeometry(x, my + 1, w, h);
    if (my == y1)
        this->setGeometry(x, my - 1, w, h);
}

void dlgScrollBox::init_ScrollBox()
{
    x = mw_one->x() + mw_one->width() - mw_one->miniEdit->width() - 4;
    w = mw_one->miniEdit->width();
    h = s_box_h;

    y0 = mw_one->y()
         + (mw_one->height() - mw_one->ui->statusbar->height() - mw_one->miniEdit->height())
         + this->height();

    int p0;
    int h0 = mw_one->miniEdit->height() - this->height();
    int h1 = mw_one->miniEdit->verticalScrollBar()->maximum();
    int p1 = mw_one->miniEdit->verticalScrollBar()->sliderPosition();
    double b = (double) (p1) / (double) (h1);
    p0 = h0 * b;
    if (p0 < 0) {
        close();
        return;
    }

    my = p0 + y0;

    this->setGeometry(x, my, w, h);
    this->show();
}

#include "MyTabPage.h"
#include "mainwindow.h"

#include <QDebug>
#include <QLabel>

//用于累加计算唯一id
int global_id = 0;
//用于保存当前存在的page实例的id
QList<int> active_id;

MyTabPage::MyTabPage(QWidget* parent)
    : QWidget(parent)
{
}

MyTabPage::~MyTabPage()
{
    active_id.removeOne(id);
}

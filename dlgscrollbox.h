#ifndef DLGSCROLLBOX_H
#define DLGSCROLLBOX_H

#include <QDialog>
#include <QMouseEvent>

namespace Ui {
class dlgScrollBox;
}

class dlgScrollBox : public QDialog {
  Q_OBJECT

 public:
  explicit dlgScrollBox(QWidget *parent = nullptr);
  ~dlgScrollBox();
  Ui::dlgScrollBox *ui;

  void init_ScrollBox();

 protected:
  void mousePressEvent(QMouseEvent *) override;
  void mouseReleaseEvent(QMouseEvent *) override;
  void mouseMoveEvent(QMouseEvent *) override;

 private:
  bool isDrag = false;
  QPoint m_position;
  int s_box_h = 25;
};

#endif  // DLGSCROLLBOX_H

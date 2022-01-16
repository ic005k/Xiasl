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

  bool isDrag = false;
  void init_ScrollBox();

 protected:
  void mousePressEvent(QMouseEvent *) override;
  void mouseReleaseEvent(QMouseEvent *) override;
  void mouseMoveEvent(QMouseEvent *) override;

  private:
  QPoint m_position;
  int s_box_h = 25;
  int s_box_x = 0;
  int y0, y1;
  int x, my, w, h;
  int thisP = 0;
};

#endif  // DLGSCROLLBOX_H

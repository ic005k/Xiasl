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

 protected:
  void mousePressEvent(QMouseEvent *) override;
  void mouseReleaseEvent(QMouseEvent *) override;
  void mouseMoveEvent(QMouseEvent *) override;

 private:
  bool isDrag = false;
  QPoint m_position;
};

#endif  // DLGSCROLLBOX_H

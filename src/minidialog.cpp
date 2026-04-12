#include "minidialog.h"

#include <QLabel>

#include "ui_minidialog.h"

extern QsciScintilla* miniDlgEdit;
extern miniDialog* miniDlg;
extern int red;

ZoomEditor::ZoomEditor(QWidget* parent) : QsciScintilla(parent) {}

miniDialog::miniDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::miniDialog) {
  ui->setupUi(this);

  setWindowFlags(Qt::FramelessWindowHint);

  ui->gridLayout_2->setMargin(0);
  miniDlgEdit = new ZoomEditor(this);
  ui->gridLayout->addWidget(miniDlgEdit);

  QLabel *lblTip = new QLabel(miniDlgEdit);
  lblTip->setText(tr("Left click: Position to the current line.") + "\n"
                  + tr("Right click: Copy the current line."));

  QColor color;

  if (red > 55)
    color.setRgb(245, 245, 245, 255);
  else
    color.setRgb(50, 50, 50, 255);

  miniDlgEdit->setPaper(color);

  miniDlgEdit->setContextMenuPolicy(Qt::NoContextMenu);
  miniDlgEdit->setMarginWidth(0, 0);
  miniDlgEdit->setMargins(0);
  miniDlgEdit->setReadOnly(1);
  miniDlgEdit->SendScintilla(QsciScintillaBase::SCI_SETCURSOR, 0, 7);

  //水平滚动棒
  miniDlgEdit->SendScintilla(QsciScintilla::SCI_SETSCROLLWIDTH, -1);
  miniDlgEdit->SendScintilla(QsciScintilla::SCI_SETSCROLLWIDTHTRACKING, false);

  miniDlgEdit->horizontalScrollBar()->setHidden(true);
  miniDlgEdit->verticalScrollBar()->setHidden(true);
}

miniDialog::~miniDialog() { delete ui; }

void ZoomEditor::mouseMoveEvent(QMouseEvent* event) {
  Q_UNUSED(event);

  miniDlg->close();
}

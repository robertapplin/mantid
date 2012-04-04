#ifndef MANTIDWIDGETS_PROCESSINGALGOWIDGET_H_
#define MANTIDWIDGETS_PROCESSINGALGOWIDGET_H_

#include <QWidget>
#include "WidgetDllOption.h"
#include "ui_ProcessingAlgoWidget.h"

namespace MantidQt
{
  namespace MantidWidgets
  {
    /** A widget containing an algorithm selector and algorithm properties list,
     * or a script editor window.
     * Allows the user to pick a processing step, either as a python script
     * or an algorithm.
     *
     * For use initially in the StartLiveDataDialog.
     */
    class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS ProcessingAlgoWidget : public QWidget
    {
      Q_OBJECT

    public:
      /// Default Constructor
      ProcessingAlgoWidget(QWidget *parent = 0);
      ~ProcessingAlgoWidget();
    private:
      /// Initialize the layout
      virtual void initLayout();

    private:
      //The form generated by Qt Designer
      Ui::ProcessingAlgoWidget ui;
    };


  }// Namespace
}// Namespace
#endif

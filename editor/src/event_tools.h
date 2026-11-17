#pragma once
#include "project.h"
#include <functional>
class QWidget;class QPlainTextEdit;class QLineEdit;
namespace cf {
QWidget *conditionTools(QLineEdit *,std::function<Project()>);
QWidget *eventTools(QPlainTextEdit *,std::function<Project()>);
QWidget *behaviorTools(QPlainTextEdit *,std::function<Project()>,std::function<bool()> isPlayer={});
}

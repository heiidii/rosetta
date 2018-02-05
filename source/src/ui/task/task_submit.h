#ifndef TASK_SUBMIT_H
#define TASK_SUBMIT_H

#include <ui/task/task.fwd.h>
#include <ui/task/util.h>

#include <QWidget>

namespace Ui {
class TaskSubmit;
}

namespace ui {
namespace task {

class FileTableWatcher : public QObject
{
	Q_OBJECT

	bool eventFilter(QObject * obj, QEvent * event) override;

	Q_SIGNAL void backspace_pressed();

public:
	using QObject::QObject;
};

class TaskSubmit : public QWidget
{
    Q_OBJECT

public:
    explicit TaskSubmit(TaskSP const &, QWidget *parent = 0);
    ~TaskSubmit();


private Q_SLOTS:
	void on_name_textChanged(QString const &);
	void on_app_activated(const QString &text);

	void on_version_textChanged(QString const &);

	void on_nstruct_valueChanged(int);

	void on_description_textChanged();

	void on_flags_textChanged();

	void on_add_files_clicked();

	void on_add_input_structure_clicked();
	void on_add_native_structure_clicked();

	void on_flags_from_file_clicked();

	void backspace_pressed_on_files();

	void s_on_queues_finished();

	void on_submit_clicked();

	void update_ui_from_task();




private:
    Ui::TaskSubmit *ui;

	TaskSP task_;

	NetworkCall queues;
};


} // namespace task
} // namespace ui

#endif // TASK_SUBMIT_H
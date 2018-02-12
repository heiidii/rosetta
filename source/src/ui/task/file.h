#pragma once

#include <ui/task/file.fwd.h>
#include <ui/task/task.fwd.h>

#include <QString>
#include <QAbstractTableModel>

namespace ui {
namespace task {


/// Generic container that represent abstract chunk of data that will be stored as file on back-end.
/// Derive new classes from it if you need to create new types of Node while re-use serialization routines
class FileMixin
{
public:

	virtual QString type() const = 0;

private:
	virtual QByteArray data() const = 0;
	virtual void data(QByteArray const &) = 0;

	//virtual QByteArray file_data() const = 0;
	//virtual void file_data(QByteArray const &) = 0;

	virtual QString file_name() const = 0;


	// return true if underlying object contain no data
	virtual bool empty() const = 0;
};


/// File node that hold concreate data with concreate file-name
class File : public FileMixin
{
public:
	explicit File() {}
	explicit File(QString const & file_name);
	explicit File(QByteArray const & file_data);
	explicit File(QString const & file_name, QByteArray const & file_data);

	QString type() const override { return "file"; };

	void init_from_file(QString const & file_name);

	// we pulling FileMixin methods into public because they now have double function as accessor to file data
	QByteArray data() const override;
	void data(QByteArray const &_file_data) override;

    //QByteArray file_data() const override { return file_data_; };
	//void file_data(QByteArray const &_file_data) override { file_data_ = _file_data; }

	QString file_name() const override { return file_name_; }
	void file_name(QString const &_file_name) { file_name_ = _file_name; }

	QString hash() const { return hash_; }
	void hash(QString const &_hash) { hash_ = _hash; }

	bool empty() const override { return file_data_.isEmpty(); }
	//bool null() const override { return file_data_.isNull(); }

	File& operator=(File&& other) noexcept;


	bool operator ==(File const &r) const;
	bool operator !=(File const &r) const { return not (*this == r); }

	// serialization
	friend QDataStream &operator<<(QDataStream &, File const&);
	friend QDataStream &operator>>(QDataStream &, File &);

private:
	QString hash_; // hash for file data if any (generated by server)
	QString file_name_; // local file name if any
	QByteArray file_data_;
};

//using Files = std::map<std::string, File>;


class FileTableModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	using QAbstractTableModel::QAbstractTableModel;

	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int	columnCount(const QModelIndex &parent = QModelIndex()) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

	void update_from_task(Task const &task);

Q_SIGNALS:
	void rename_file(QString const &previous_value, QString const &s);

private:
	struct Row {
		QString name, path;
	};

	std::vector<Row> rows_;
	bool editable_ = false;
};

} // namespace task
} // namespace ui
#ifndef FILEOBJECT_H
#define FILEOBJECT_H

#include <QObject>
#include <QtQml>


class WFileObject : public QObject
{
    Q_OBJECT
	Q_PROPERTY(QString selectedPath READ selectedPath WRITE setSelectedPath /*NOTIFY selectedPathChanged*/)
	Q_PROPERTY(FILEOBJECTHANDLETYPE handleType READ handleType WRITE setHandleType /*NOTIFY handleTypeChanged*/)
    Q_ENUMS(FILEOBJECTHANDLETYPE)
    QML_ELEMENT
public:
    explicit WFileObject(QObject* parent = nullptr) :QObject(parent) {}
	enum FILEOBJECTHANDLETYPE
	{
		OPENFILE = 0,
		OPENFILES,
		OPENDIR,
		SAVEFILE,
		SAVEDIR,
	};

    Q_INVOKABLE void handleFile() { emit wtFileHandle(m_selectedPath,m_fileObjectHandleType); };

    void setSelectedPath(QString& selectedPath) { this->m_selectedPath = selectedPath; }
    QString selectedPath() { return m_selectedPath; }

    void setHandleType(FILEOBJECTHANDLETYPE handleType) { this->m_fileObjectHandleType = handleType; }
    FILEOBJECTHANDLETYPE handleType() { return m_fileObjectHandleType; }

signals:
    void wtFileHandle(QString selectedPath,FILEOBJECTHANDLETYPE type);
//	void selectedPathChanged(QString& filePath);
//	void handleTypeChanged(FILEOBJECTHANDLETYPE handleType);

private:
    QString m_selectedPath;
    FILEOBJECTHANDLETYPE m_fileObjectHandleType;
};

#endif // FILEOBJECT_H

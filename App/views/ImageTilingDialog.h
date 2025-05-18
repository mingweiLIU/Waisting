#ifndef TILEPROCESSOR_H
#define TILEPROCESSOR_H

#include <QObject>
#include <QVariantList>
#include <QQmlEngine>
#include <QJSEngine>
#include <QString>
#include <QFuture>
#include <QFutureWatcher>
#include <QVector>
#include <QtConcurrent/QtConcurrent>
#include "../../WTFrame/IProgressInfo.h"

/**
 * @brief 切片处理器类，处理栅格数据切片操作
 * 
 * 该类提供与QML前端交互的接口，用于处理栅格数据的切片操作，
 * 并通过信号传递处理进度和结果。
 */
class ImageTilingDialog : public QObject
{
    Q_OBJECT
    
    // 属性定义，与QML双向绑定
    Q_PROPERTY(QString inputFile READ getInputFile WRITE setInputFile NOTIFY inputFileChanged)
    Q_PROPERTY(QString outputDir READ getOutputDir WRITE setOutputDir NOTIFY outputDirChanged)
    Q_PROPERTY(int minLevel READ getMinLevel WRITE setMinLevel NOTIFY minLevelChanged)
    Q_PROPERTY(int maxLevel READ getMaxLevel WRITE setMaxLevel NOTIFY maxLevelChanged)
    Q_PROPERTY(QVariantList nodata READ getNodata WRITE setNodata NOTIFY nodataChanged)
    Q_PROPERTY(QString outputFormat READ getOutputFormat WRITE setOutputFormat NOTIFY outputFormatChanged)
    Q_PROPERTY(QString prjFilePath READ getPrjFilePath WRITE setPrjFilePath NOTIFY prjFilePathChanged)
    Q_PROPERTY(QString wktString READ getWktString WRITE setWktString NOTIFY wktStringChanged)
    Q_PROPERTY(int coordinateSystemType READ getCoordinateSystemType WRITE setCoordinateSystemType NOTIFY coordinateSystemTypeChanged)
    Q_PROPERTY(double progress READ getProgress NOTIFY progressChanged)
    Q_PROPERTY(bool isProcessing READ getIsProcessing NOTIFY isProcessingChanged)
    
public:
    explicit ImageTilingDialog(QObject *parent = nullptr);
    ~ImageTilingDialog();
    
    // 属性访问方法
    QString getInputFile() const { return m_inputFile; }
    void setInputFile(const QString &file);
    
    QString getOutputDir() const { return m_outputDir; }
    void setOutputDir(const QString &dir);
    
    int getMinLevel() const { return m_minLevel; }
    void setMinLevel(int level);
    
    int getMaxLevel() const { return m_maxLevel; }
    void setMaxLevel(int level);
    
    QVariantList getNodata() const { return m_nodata; }
    void setNodata(const QVariantList &nodata);
    
    QString getOutputFormat() const { return m_outputFormat; }
    void setOutputFormat(const QString &format);
    
    QString getPrjFilePath() const { return m_prjFilePath; }
    void setPrjFilePath(const QString &path);
    
    QString getWktString() const { return m_wktString; }
    void setWktString(const QString &wkt);
    
    int getCoordinateSystemType() const { return m_coordinateSystemType; }
    void setCoordinateSystemType(int type);
    
    double getProgress() const { return m_progress; }
    
    bool getIsProcessing() const { return m_isProcessing; }
    
    // 单例模式，用于QML注册
    static QObject* singletonProvider(QQmlEngine *engine, QJSEngine *scriptEngine);
    
public slots:
    // 启动切片处理
    bool startProcessing();
    
    // 取消当前处理
	void cancelProcessing();
	// 处理进度更新
	void updateProgress(double progress);
    
signals:
    // 属性变更信号
    void inputFileChanged();
    void outputDirChanged();
    void minLevelChanged();
    void maxLevelChanged();
    void nodataChanged();
    void outputFormatChanged();
    void prjFilePathChanged();
    void wktStringChanged();
    void coordinateSystemTypeChanged();
    void progressChanged();
    void isProcessingChanged();
    
    // 处理结果信号
    void processingFinished(bool success);
    void processingError(const QString &errorMessage);
    
private slots:
    
    // 处理完成
    void handleProcessingFinished();
    
private:
    // 属性成员
    QString m_inputFile;
    QString m_outputDir;
    int m_minLevel;
    int m_maxLevel;
    QVariantList m_nodata;
    QString m_outputFormat;
    QString m_prjFilePath;
    QString m_wktString;
    int m_coordinateSystemType;  // 0=影像自带坐标, 1=PRJ文件, 2=WKT字符串
    double m_progress;
    bool m_isProcessing;
    
    // 异步处理相关
    QFutureWatcher<bool> *m_futureWatcher;
    bool m_cancelRequested;
    
    // 参数验证方法
    bool validateParameters();
    
    // 内部切片处理方法
    bool processTiles();
};

//处理进度条的类
class  ImageTilingDialogProgress final :public  WT::IProgressInfo  {
private:
    ImageTilingDialog* dialog = nullptr;
public:
    ImageTilingDialogProgress(ImageTilingDialog* dialog) :dialog(dialog){ }
    /*****************************************************************************
	* @brief : 显示处理进度
	*****************************************************************************/
    void showProgress(int currentIndex, std::string currentFileName, std::string operats = "数据转换") {
        dialog->updateProgress(currentIndex / (totalNum * 1.0f));
    }
    /*****************************************************************************
    * @brief : 完成进度
    *****************************************************************************/
    void finished(std::string label = "处理完成") const {
        dialog->updateProgress(1.0);
    }
};

#endif // TILEPROCESSOR_H
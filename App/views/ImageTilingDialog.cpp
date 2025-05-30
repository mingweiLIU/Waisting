#include "ImageTilingDialog.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <vector>
#include "../../WTDataManipulate/IDataM.h"

// 构造函数
ImageTilingDialog::ImageTilingDialog(QObject *parent)
    : QObject(parent)
    , m_minLevel(15)
    , m_maxLevel(18)
    , m_coordinateSystemType(0)
    , m_progress(0.0)
    , m_isProcessing(false)
    , m_cancelRequested(false)
{
    // 初始化默认nodata值
    m_nodata << 0.0 << 0.0 << 0.0;
    
    // 初始化默认输出格式
    m_outputFormat = "png";
    
    // 创建Future监视器
    m_futureWatcher = new QFutureWatcher<bool>(this);
    
    // 连接信号槽
    connect(m_futureWatcher, &QFutureWatcher<bool>::finished, this, &ImageTilingDialog::handleProcessingFinished);
}

// 析构函数
ImageTilingDialog::~ImageTilingDialog()
{
    // 确保在析构时取消任何未完成的处理
    if (m_isProcessing) {
        cancelProcessing();
    }
    
    delete m_futureWatcher;
}

// 属性设置方法实现
void ImageTilingDialog::setInputFile(const QString &file)
{
    if (m_inputFile != file) {
        m_inputFile = file;
        emit inputFileChanged();
    }
}

void ImageTilingDialog::setOutputDir(const QString &dir)
{
    if (m_outputDir != dir) {
        m_outputDir = dir;
        emit outputDirChanged();
    }
}

void ImageTilingDialog::setMinLevel(int level)
{
    if (m_minLevel != level) {
        m_minLevel = level;
        emit minLevelChanged();
    }
}

void ImageTilingDialog::setMaxLevel(int level)
{
    if (m_maxLevel != level) {
        m_maxLevel = level;
        emit maxLevelChanged();
    }
}

void ImageTilingDialog::setNodata(const QVariantList &nodata)
{
    if (m_nodata != nodata) {
        m_nodata = nodata;
        emit nodataChanged();
    }
}

void ImageTilingDialog::setOutputFormat(const QString &format)
{
    if (m_outputFormat != format) {
        m_outputFormat = format;
        emit outputFormatChanged();
    }
}

void ImageTilingDialog::setPrjFilePath(const QString &path)
{
    if (m_prjFilePath != path) {
        m_prjFilePath = path;
        emit prjFilePathChanged();
    }
}

void ImageTilingDialog::setWktString(const QString &wkt)
{
    if (m_wktString != wkt) {
        m_wktString = wkt;
        emit wktStringChanged();
    }
}

void ImageTilingDialog::setCoordinateSystemType(int type)
{
    if (m_coordinateSystemType != type) {
        m_coordinateSystemType = type;
        emit coordinateSystemTypeChanged();
    }
}

// 单例模式提供者，用于QML注册
QObject* ImageTilingDialog::singletonProvider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)
    
    // 创建单例实例
    ImageTilingDialog *processor = new ImageTilingDialog();
    
    // QML引擎接管该对象的所有权
    return processor;
}

// 启动处理
bool ImageTilingDialog::startProcessing()
{
    // 如果正在处理，则返回失败
    if (m_isProcessing) {
        qWarning() << "切片处理已在进行中，请等待当前处理完成";
        return false;
    }
    
    // 验证参数
    if (!validateParameters()) {
        return false;
    }
    
    // 重置取消标志
    m_cancelRequested = false;
    
    // 设置处理状态
    m_isProcessing = true;
    emit isProcessingChanged();
    
    // 设置初始进度
    m_progress = 0.0;
    emit progressChanged();
    
    // 在单独的线程中启动处理
    QFuture<bool> future = QtConcurrent::run([this]() { return this->processTiles(); });
    m_futureWatcher->setFuture(future);
    
    return true;
}

// 取消处理
void ImageTilingDialog::cancelProcessing()
{
    if (m_isProcessing) {
        m_cancelRequested = true;

        mapTiler->cancle();
        
        // 等待线程完成
        m_futureWatcher->waitForFinished();

        // 重置状态
        m_isProcessing = false;
        emit isProcessingChanged();
        
        m_progress = 0.0;
        emit progressChanged();
        
        qDebug() << "切片处理已取消";
    }
}

// 更新进度
void ImageTilingDialog::updateProgress(double progress)
{
    if (m_progress != progress) {
        m_progress = progress;
        emit progressChanged();
    }
}

// 处理完成处理器
void ImageTilingDialog::handleProcessingFinished()
{
    bool success = m_futureWatcher->result();
    
    // 重置处理状态
    m_isProcessing = false;
    emit isProcessingChanged();
    
    // 处理成功与否
    if (success && !m_cancelRequested) {
        m_progress = 1.0;
        emit progressChanged();
        emit processingFinished(true);
        qDebug() << "切片处理成功完成";
    } else if (m_cancelRequested) {
        m_progress = 0.0;
        emit progressChanged();
        emit processingError("处理被用户取消");
        qDebug() << "切片处理被用户取消";
    } else {
        m_progress = 0.0;
        emit progressChanged();
        emit processingError("切片处理失败");
        qDebug() << "切片处理失败";
    }
}

// 验证参数
bool ImageTilingDialog::validateParameters()
{
    // 检查输入文件
    if (m_inputFile.isEmpty()) {
        emit processingError("请选择输入文件");
        return false;
    }
    
    QFileInfo inputFileInfo(m_inputFile);
    if (!inputFileInfo.exists() || !inputFileInfo.isReadable()) {
        emit processingError("输入文件不存在或无法读取");
        return false;
    }
    
    // 检查输出目录
    if (m_outputDir.isEmpty()) {
        emit processingError("请选择输出目录");
        return false;
    }
    
    QDir outputDir(m_outputDir);
    if (!outputDir.exists()) {
        // 尝试创建输出目录
        if (!outputDir.mkpath(".")) {
            emit processingError("输出目录不存在且无法创建");
            return false;
        }
    }
    
    // 检查坐标系统设置
    if (m_coordinateSystemType == 1 && m_prjFilePath.isEmpty()) {
        emit processingError("选择使用PRJ文件时，必须指定PRJ文件路径");
        return false;
    }
    
    if (m_coordinateSystemType == 1) {
        QFileInfo prjFileInfo(m_prjFilePath);
        if (!prjFileInfo.exists() || !prjFileInfo.isReadable()) {
            emit processingError("PRJ文件不存在或无法读取");
            return false;
        }
    }
    
    if (m_coordinateSystemType == 2 && m_wktString.isEmpty()) {
        emit processingError("选择使用WKT字符串时，必须输入有效的WKT字符串");
        return false;
    }

    if (m_nodata.size()>0 && m_nodata.size()<3)
	{
		emit processingError("Nodata要么不设置，要么三个波段都需要设置，如果数据波段数为1，其他g/b分量随意设置即可");
		return false;
    }

    //在nodata某个值写入数字后 再删除 那么这个size会存在 但是null不存在
    for (int i=0;i< m_nodata.size();++i)
    {
        if (m_nodata[i].isNull()) {
			emit processingError("Nodata要么不设置，要么三个波段都需要设置，如果数据波段数为1，其他g/b分量随意设置即可");
			return false;
        }
    }

    
    return true;
}

// 切片处理实现
bool ImageTilingDialog::processTiles()
{
    // 这里实现实际的切片处理逻辑    
    // 模拟处理进度
    //for (int i = 1; i <= 100; i++) {
    //    // 检查是否请求取消
    //    if (m_cancelRequested) {
    //        return false;
    //    }
    //    
    //    // 更新进度
    //    double progress = i / 100.0;
    //    QMetaObject::invokeMethod(this, "updateProgress", Qt::QueuedConnection, Q_ARG(double, progress));
    //    
    //    // 模拟处理时间
    //    QThread::msleep(50); // 每步延迟50毫秒
    //}

	std::shared_ptr< WT::SlippyMapTilerOptions> options = std::make_shared<WT::SlippyMapTilerOptions>();
	options->inputFile = m_inputFile.toStdString();
	options->outputDir = m_outputDir.toStdString();
    options->minLevel = m_minLevel;
    options->maxLevel = m_maxLevel;
    
    std::string formatStr = m_outputFormat.toStdString();
    if ("png"==formatStr)
	{
		options->outputFormat = WT::IMAGEFORMAT::PNG;
    }else if ("jpg" == formatStr)
	{
		options->outputFormat = WT::IMAGEFORMAT::JPG;
    }
    else {
		emit processingError("设置的输出格式暂时未支持！");
		return false;
    }


    options->prjFilePath = m_prjFilePath.toStdString();
    options->wktString = m_wktString.toStdString();

    std::vector<double>().swap(options->nodata);
	std::transform(m_nodata.begin(), m_nodata.end(), std::back_inserter(options->nodata),
		[](const QVariant& item) {
            if (item.isValid())
			{
				return item.toDouble(); // 无错误检查
            }
	});

	std::shared_ptr<ImageTilingDialogProgress> imageTilingDialogProgress = std::make_shared<ImageTilingDialogProgress>(this);
    mapTiler=std::make_shared<WT::SlippyMapTiler>(options);

	// 初始化
	if (!mapTiler->initialize()) {
		std::cerr << "初始化错误" << std::endl;
		emit processingError("初始化失败！");
		return false;
	}

	// 处理切片
	std::cout << "开始生成切片..." << std::endl;
	auto start_time = std::chrono::high_resolution_clock::now();

    try
	{
		bool success = mapTiler->process(imageTilingDialogProgress);
		auto end_time = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();
		return true; // 返回处理结果
    }
    catch (...)
    {
        return false;
    }    
}
# R8j step 3: heaptrack attribution per cycle window

Data: `/data/tmp/r8j-20261110/pilot/WH2/heaptrack/heaptrack.zst`. Run: `/data/tmp/r8j-20261110/pilot/WH2`. Memory maps: /data/tmp/r8j-20261110/pilot/WH2/session/maps-end.txt, /data/tmp/r8j-20261110/pilot/WH2/session/maps-warmup.txt.
Records: 4404 strings, 34244 instruction records, 1194423 trace nodes, 286286 allocation infos, 4290032 allocation and free events; unknown infos 0, unparsed lines 0. Last heaptrack time 140471 ms.

## Totals per cycle window (MiB)

| cycle | allocated | freed | net | synchronized release | transient (estimate) |
|---:|---:|---:|---:|---:|---:|
| 1 | 148.92 | 148.53 | 0.39 | 4.08 | 148.36 |

## Cycle 1: sites by their share of the synchronized release (top 15)

Attribution set (covers 50% of the positive shares): 1 sites; share of the positive shares: 69.1%.

| site (innermost first) | share MiB | net MiB | allocations | frees | first dock frame |
|---|---:|---:|---:|---:|---|
| _glDeterministicTPCModeDistParams+0xfb594 < ??@libnvidia-eglcore.so.580.178.04 < ??@libnvidia-eglcore.so.580.178.04 | 2.844 | 0.036 | 4104 | 4066 | main+0xf32 |
| operator new(unsigned long)+0x1b < QSGBatchRenderer::Renderer::Renderer(QSGDefaultRenderContext*, QSGRendererInterface::RenderMode)+0x4af < QSGRhiLayer::grab()+0xaa2 | 0.073 | 0.000 | 5 | 5 | main+0xf32 |
| operator new(unsigned long)+0x1b < QQmlType::create(void**, unsigned long) const+0x34 < QQmlObjectCreator::createInstance(int, QObject*, bool)+0x26a | 0.061 | 0.000 | 300 | 300 | main+0xf32 |
| operator new(unsigned long)+0x1b < QObjectPrivate::connectImpl(QObject const*, int, QObject const*, void**, QtPrivate::QSlotObjectBase*, int, int const*, QMetaObject const*)+0x13d < QObject::connectImpl(QObject const*, void**, QObject const*, void**, QtPrivate::QSlotObjectBase*, Qt::ConnectionType, int const*, QMetaObject const*)+0x104 | 0.040 | -0.017 | 306 | 504 | main+0xf32 |
| _glDeterministicTPCModeDistParams+0xfbec0 < NvGlEglGetFunctions+0x31537 < ??@libnvidia-eglcore.so.580.178.04 | 0.035 | 0.000 | 5 | 5 | main+0xf32 |
| operator new(unsigned long)+0x1b < QSGBatchRenderer::Renderer::Renderer(QSGDefaultRenderContext*, QSGRendererInterface::RenderMode)+0x5a7 < QSGRhiLayer::grab()+0xaa2 | 0.028 | 0.000 | 5 | 5 | main+0xf32 |
| operator new(unsigned long)+0x1b < QQmlBinding::newBinding(QMetaType)+0x201 < QQmlBinding::create(QMetaType, QV4::Function*, QObject*, QQmlRefPointer<QQmlContextData> const&, QV4::ExecutionContext*)+0x1c | 0.025 | 0.000 | 211 | 211 | main+0xf32 |
| _glDeterministicTPCModeDistParams+0xfbec0 < _glDeterministicTPCModeDistParams+0xf0297 < _glDeterministicTPCModeDistParams+0xe8ff0 | 0.024 | 0.000 | 5 | 5 | main+0xf32 |
| operator new(unsigned long)+0x1b < void std::vector<QQmlComponentPrivate::ConstructionState, std::allocator<QQmlComponentPrivate::ConstructionState> >::_M_realloc_append<QQmlComponentPrivate::ConstructionState>(QQmlComponentPrivate::ConstructionState&&)+0x6e8c < QQmlData::deferData(int, QQmlRefPointer<QV4::ExecutableCompilationUnit> const&, QQmlRefPointer<QQmlContextData> const&, QString const&)+0x336 | 0.023 | 0.000 | 42 | 42 | main+0xf32 |
| _glDeterministicTPCModeDistParams+0xfbec0 < NvGlEglGetFunctions+0x32939 < ??@libnvidia-eglcore.so.580.178.04 | 0.020 | 0.000 | 15 | 15 | main+0xf32 |
| operator new(unsigned long)+0x1b < QQmlPropertyBinding::create(QMetaType, QV4::Function*, QObject*, QQmlRefPointer<QQmlContextData> const&, QV4::ExecutionContext*, QObject*, QQmlPropertyIndex)+0x4b < QQmlObjectCreator::setPropertyBinding(QQmlPropertyData const*, QV4::CompiledData::Binding const*)+0xba4 | 0.019 | 0.000 | 70 | 70 | main+0xf32 |
| operator new(unsigned long)+0x1b < QQmlBinding::newBinding(QMetaType)+0x173 < QQmlBinding::create(QMetaType, QV4::Function*, QObject*, QQmlRefPointer<QQmlContextData> const&, QV4::ExecutionContext*)+0x1c | 0.018 | 0.000 | 139 | 139 | main+0xf32 |
| operator new(unsigned long)+0x1b < QSGPlainTexture::qt_metacall(QMetaObject::Call, int, void**)+0xaf29 < QSGCurveGlyphAtlas::~QSGCurveGlyphAtlas()+0x1997 | 0.018 | 0.000 | 8 | 8 | main+0xf32 |
| operator new(unsigned long)+0x1b < QSGRhiLayer::grab()+0xa91 < QSGRhiLayer::updateTexture()+0x117 | 0.017 | 0.000 | 5 | 5 | main+0xf32 |
| operator new(unsigned long)+0x1b < QQmlBinding::newBinding(QMetaType)+0x25 < QQmlBinding::create(QMetaType, QV4::Function*, QObject*, QQmlRefPointer<QQmlContextData> const&, QV4::ExecutionContext*)+0x1c | 0.017 | 0.000 | 127 | 127 | main+0xf32 |

## Cycle 1: sites with net live growth (top 15)

| site | net MiB | allocations | frees |
|---|---:|---:|---:|
| QImageData::create(QSize const&, QImage::Format)+0x1bc < QImage::QImage(int, int, QImage::Format)+0x3c < qt_blurImage(QImage&, double, bool, int)+0x3a5b | 0.122 | 15 | 0 |
| QQmlPropertyCapture::captureProperty(QQmlNotifier*)+0x2cd < QV4::QQmlContextWrapper::lookupIdObject(QV4::Lookup*, QV4::ExecutionEngine*, QV4::Value*)+0xb1 < ??@?? | 0.055 | 1 | 0 |
| QQmlPropertyCapture::captureNonBindableProperty(QObject*, int, int, bool)+0x72e < QV4::QObjectWrapper::getProperty(QV4::ExecutionEngine*, QV4::Heap::Object*, QObject*, QQmlPropertyData const*, QFlags<QV4::QObjectWrapper::Flag>)+0xd2 < QV4::QObjectWrapper::getQmlProperty(QV4::ExecutionEngine*, QQmlRefPointer<QQmlContextData> const&, QV4::Heap::Object*, QObject*, QV4::String*, QFlags<QV4::QObjectWrapper::Flag>, bool*, QQmlPropertyData const**)+0x1d6 | 0.055 | 1 | 0 |
| _glDeterministicTPCModeDistParams+0xfb594 < ??@libnvidia-eglcore.so.580.178.04 < ??@libnvidia-eglcore.so.580.178.04 | 0.036 | 4104 | 4066 |
| QByteArray::reallocData(long long, QArrayData::AllocationOption)+0x8f < QByteArray::resize(long long)+0x49 < operator>>(QDataStream&, QByteArray&)+0xdb | 0.034 | 36 | 0 |
| QImageData::create(QSize const&, QImage::Format)+0x1bc < QImage::QImage(QSize const&, QImage::Format)+0x2a < QImageIOHandler::allocateImage(QSize, QImage::Format, QImage*)+0xf5 | 0.031 | 2 | 0 |
| operator new(unsigned long)+0x1b < QOpenGLProgramBinaryCache::save(QByteArray const&, unsigned int)+0x3495 < QSGBatchRenderer::Renderer::ensurePipelineState(QSGBatchRenderer::Element*, QSGBatchRenderer::ShaderManagerShader const*, bool)+0x353 | 0.021 | 1 | 0 |
| _glDeterministicTPCModeDistParams+0xfbec0 < ??@libnvidia-eglcore.so.580.178.04 < ??@libnvidia-eglcore.so.580.178.04 | 0.012 | 202 | 162 |
| QByteArray::QByteArray(long long, Qt::Initialization)+0x3a < QRhiGles2InitParams::QRhiGles2InitParams()+0x11797 < QRhiGles2InitParams::QRhiGles2InitParams()+0x173e7 | 0.010 | 1 | 0 |
| operator new(unsigned long)+0x1b < QOpenGLProgramBinaryCache::save(QByteArray const&, unsigned int)+0x2be5 < QSGBatchRenderer::Renderer::updateMaterialDynamicData(QSGBatchRenderer::ShaderManagerShader*, QSGMaterialShader::RenderState&, QSGMaterial*, QSGBatchRenderer::Batch const*, QSGBatchRenderer::Element*, int, int, char*)+0x12f0 | 0.010 | 8 | 5 |
| QByteArray::QByteArray(char const*, long long)+0x65 < QByteArray::assign(QByteArrayView)+0x5e < QSGBatchRenderer::Renderer::unmap(QSGBatchRenderer::Buffer*, bool)+0xa0 | 0.007 | 4 | 0 |
| _glDeterministicTPCModeDistParams+0xfbec0 < NvGlEglGetFunctions+0x5b376 < NvGlEglGetFunctions+0x5e42a | 0.007 | 63 | 28 |
| operator new(unsigned long)+0x1b < QRhiResource::deleteLater()+0x9516 < QRhiResource::deleteLater()+0x9755 | 0.006 | 8 | 0 |
| operator new(unsigned long)+0x1b < QSGMaterialShader::QSGMaterialShader()+0x23 < QSGDefaultInternalImageNode::QSGDefaultInternalImageNode(QSGDefaultRenderContext*)+0xe6 | 0.004 | 1 | 0 |
| operator new(unsigned long)+0x1b < QSGBatchRenderer::Renderer::prepareInline()+0x167fc < QSGBatchRenderer::Renderer::nodeWasAdded(QSGNode*, QSGBatchRenderer::Node*)+0x2b9 | 0.004 | 3 | 0 |


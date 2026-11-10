# R8j step 3: heaptrack attribution per cycle window

Data: `/data/tmp/r8j-20261110/runs/WH/heaptrack/heaptrack.zst`. Run: `/data/tmp/r8j-20261110/runs/WH`. Memory maps: /data/tmp/r8j-20261110/runs/WH/session/maps-end.txt, /data/tmp/r8j-20261110/runs/WH/session/maps-warmup.txt.
Records: 4406 strings, 34290 instruction records, 1216977 trace nodes, 293389 allocation infos, 19582064 allocation and free events; unknown infos 0, unparsed lines 0. Last heaptrack time 1250337 ms.

## Totals per cycle window (MiB)

| cycle | allocated | freed | net | synchronized release | transient (estimate) |
|---:|---:|---:|---:|---:|---:|
| 1 | 150.95 | 150.56 | 0.39 | 4.08 | 150.38 |
| 2 | 142.99 | 142.75 | 0.25 | 2.12 | 142.73 |
| 3 | 138.91 | 138.67 | 0.23 | 2.13 | 138.67 |
| 4 | 139.13 | 139.03 | 0.10 | 2.12 | 138.98 |
| 5 | 138.97 | 139.21 | -0.24 | 2.12 | 138.94 |
| 6 | 140.93 | 140.82 | 0.11 | 2.12 | 140.81 |
| 7 | 138.88 | 138.79 | 0.09 | 2.13 | 138.76 |
| 8 | 138.88 | 138.82 | 0.06 | 2.13 | 138.78 |
| 9 | 138.98 | 139.22 | -0.24 | 2.12 | 138.95 |
| 10 | 138.89 | 138.79 | 0.09 | 2.13 | 138.78 |
| 11 | 138.88 | 138.75 | 0.13 | 2.21 | 138.74 |
| 12 | 139.90 | 169.72 | -29.82 | 32.17 | 139.69 |

## Cycle 1: sites by their share of the synchronized release (top 15)

Attribution set (covers 50% of the positive shares): 1 sites; share of the positive shares: 69.1%.

| site (innermost first) | share MiB | net MiB | allocations | frees | first dock frame |
|---|---:|---:|---:|---:|---|
| _glDeterministicTPCModeDistParams+0xfb594 < ??@libnvidia-eglcore.so.580.178.04 < ??@libnvidia-eglcore.so.580.178.04 | 2.844 | 0.037 | 4104 | 4065 | main+0xf32 |
| operator new(unsigned long)+0x1b < QSGBatchRenderer::Renderer::Renderer(QSGDefaultRenderContext*, QSGRendererInterface::RenderMode)+0x4af < QSGRhiLayer::grab()+0xaa2 | 0.073 | 0.000 | 5 | 5 | main+0xf32 |
| operator new(unsigned long)+0x1b < QQmlType::create(void**, unsigned long) const+0x34 < QQmlObjectCreator::createInstance(int, QObject*, bool)+0x26a | 0.061 | 0.000 | 300 | 300 | main+0xf32 |
| operator new(unsigned long)+0x1b < QObjectPrivate::connectImpl(QObject const*, int, QObject const*, void**, QtPrivate::QSlotObjectBase*, int, int const*, QMetaObject const*)+0x13d < QObject::connectImpl(QObject const*, void**, QObject const*, void**, QtPrivate::QSlotObjectBase*, Qt::ConnectionType, int const*, QMetaObject const*)+0x104 | 0.040 | -0.017 | 306 | 504 | _ZN5Latte13ShortcutsPart16ShortcutsTracker20parseGlobalShortcutsEv+0x947 |
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
| QImageData::create(QSize const&, QImage::Format)+0x1bc < QImage::QImage(int, int, QImage::Format)+0x3c < qt_blurImage(QImage&, double, bool, int)+0x3a5b | 0.126 | 17 | 1 |
| QQmlPropertyCapture::captureProperty(QQmlNotifier*)+0x2cd < QV4::QQmlContextWrapper::lookupIdObject(QV4::Lookup*, QV4::ExecutionEngine*, QV4::Value*)+0xb1 < ??@?? | 0.055 | 1 | 0 |
| QQmlPropertyCapture::captureNonBindableProperty(QObject*, int, int, bool)+0x72e < QV4::QObjectWrapper::getProperty(QV4::ExecutionEngine*, QV4::Heap::Object*, QObject*, QQmlPropertyData const*, QFlags<QV4::QObjectWrapper::Flag>)+0xd2 < QV4::QObjectWrapper::getQmlProperty(QV4::ExecutionEngine*, QQmlRefPointer<QQmlContextData> const&, QV4::Heap::Object*, QObject*, QV4::String*, QFlags<QV4::QObjectWrapper::Flag>, bool*, QQmlPropertyData const**)+0x1d6 | 0.055 | 1 | 0 |
| _glDeterministicTPCModeDistParams+0xfb594 < ??@libnvidia-eglcore.so.580.178.04 < ??@libnvidia-eglcore.so.580.178.04 | 0.037 | 4104 | 4065 |
| QByteArray::reallocData(long long, QArrayData::AllocationOption)+0x8f < QByteArray::resize(long long)+0x49 < operator>>(QDataStream&, QByteArray&)+0xdb | 0.034 | 36 | 0 |
| QImageData::create(QSize const&, QImage::Format)+0x1bc < QImage::QImage(QSize const&, QImage::Format)+0x2a < QImageIOHandler::allocateImage(QSize, QImage::Format, QImage*)+0xf5 | 0.031 | 2 | 0 |
| operator new(unsigned long)+0x1b < QOpenGLProgramBinaryCache::save(QByteArray const&, unsigned int)+0x3495 < QSGBatchRenderer::Renderer::ensurePipelineState(QSGBatchRenderer::Element*, QSGBatchRenderer::ShaderManagerShader const*, bool)+0x353 | 0.021 | 1 | 0 |
| _glDeterministicTPCModeDistParams+0xfbec0 < ??@libnvidia-eglcore.so.580.178.04 < ??@libnvidia-eglcore.so.580.178.04 | 0.012 | 202 | 162 |
| QByteArray::QByteArray(long long, Qt::Initialization)+0x3a < QRhiGles2InitParams::QRhiGles2InitParams()+0x11797 < QRhiGles2InitParams::QRhiGles2InitParams()+0x173e7 | 0.010 | 1 | 0 |
| operator new(unsigned long)+0x1b < QOpenGLProgramBinaryCache::save(QByteArray const&, unsigned int)+0x2be5 < QSGBatchRenderer::Renderer::updateMaterialDynamicData(QSGBatchRenderer::ShaderManagerShader*, QSGMaterialShader::RenderState&, QSGMaterial*, QSGBatchRenderer::Batch const*, QSGBatchRenderer::Element*, int, int, char*)+0x12f0 | 0.010 | 8 | 5 |
| QByteArray::QByteArray(char const*, long long)+0x65 < QByteArray::assign(QByteArrayView)+0x5e < QSGBatchRenderer::Renderer::unmap(QSGBatchRenderer::Buffer*, bool)+0xa0 | 0.007 | 4 | 0 |
| operator new(unsigned long)+0x1b < QSGBatchRenderer::Renderer::prepareInline()+0x167fc < QSGBatchRenderer::Renderer::nodeWasAdded(QSGNode*, QSGBatchRenderer::Node*)+0x2b9 | 0.006 | 5 | 0 |
| operator new(unsigned long)+0x1b < QRhiResource::deleteLater()+0x9516 < QRhiResource::deleteLater()+0x9755 | 0.006 | 8 | 0 |
| operator new(unsigned long)+0x1b < QSGMaterialShader::QSGMaterialShader()+0x23 < QSGDefaultInternalImageNode::QSGDefaultInternalImageNode(QSGDefaultRenderContext*)+0xe6 | 0.004 | 1 | 0 |
| _glDeterministicTPCModeDistParams+0xfb594 < NvGlEglGetFunctions+0x9b7f < ??@libnvidia-eglcore.so.580.178.04 | 0.004 | 42 | 36 |

## Reuse check: windows 2 to 10 against the window-1 attribution set

| cycle | release MiB | share from the window-1 set | net MiB from new sites (positive) |
|---:|---:|---:|---:|
| 2 | 2.130 | 0.0% | 0.250 |
| 3 | 2.130 | 0.0% | 0.237 |
| 4 | 2.130 | 0.0% | 0.118 |
| 5 | 2.129 | 0.0% | 0.018 |
| 6 | 2.130 | 0.0% | 0.108 |
| 7 | 2.130 | 0.0% | 0.092 |
| 8 | 2.130 | 0.0% | 0.088 |
| 9 | 2.129 | 0.0% | 0.002 |
| 10 | 2.130 | 0.0% | 0.095 |


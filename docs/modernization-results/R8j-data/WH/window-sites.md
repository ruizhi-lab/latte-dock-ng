# R8j step 3: sites of windows 2 to 10 (supplementary)

Positive net outside the window-1 set, summed over windows 2 to 10: 1.007 MiB. Total net of windows 2 to 10 (all sites): 0.451 MiB.

| site (innermost first) | positive net outside the set, MiB | share of the releases of windows 2 to 10, MiB |
|---|---:|---:|
| QArrayData::reallocateUnaligned(QArrayData*, void*, long long, long long, QArrayData::AllocationOption)+0x56 < std::vector<QQuickBasePositioner::PositionedItem, std::allocator<QQuickBasePositioner::PositionedItem> >::reserve(unsigned long)+0x7dcc < non-virtual thunk to QQuickTreeView::~QQuickTreeView()+0x13c | 0.000 | 9.000 |
| operator new(unsigned long)+0x1b < QSGBatchRenderer::Renderer::Renderer(QSGDefaultRenderContext*, QSGRendererInterface::RenderMode)+0x4af < QSGRhiLayer::grab()+0xaa2 | 0.000 | 0.661 |
| operator new(unsigned long)+0x1b < QQmlType::create(void**, unsigned long) const+0x34 < QQmlObjectCreator::createInstance(int, QObject*, bool)+0x26a | 0.000 | 0.560 |
| _glDeterministicTPCModeDistParams+0xfbec0 < NvGlEglGetFunctions+0x31537 < ??@libnvidia-eglcore.so.580.178.04 | 0.000 | 0.316 |
| QImageData::create(QSize const&, QImage::Format)+0x1bc < QImage::QImage(int, int, QImage::Format)+0x3c < qt_blurImage(QImage&, double, bool, int)+0x3a5b | 0.277 | 0.000 |
| operator new(unsigned long)+0x1b < QSGBatchRenderer::Renderer::Renderer(QSGDefaultRenderContext*, QSGRendererInterface::RenderMode)+0x5a7 < QSGRhiLayer::grab()+0xaa2 | 0.000 | 0.254 |
| operator new(unsigned long)+0x1b < QQmlBinding::newBinding(QMetaType)+0x201 < QQmlBinding::create(QMetaType, QV4::Function*, QObject*, QQmlRefPointer<QQmlContextData> const&, QV4::ExecutionContext*)+0x1c | 0.000 | 0.233 |
| _glDeterministicTPCModeDistParams+0xfbec0 < _glDeterministicTPCModeDistParams+0xf0297 < _glDeterministicTPCModeDistParams+0xe8ff0 | 0.000 | 0.216 |
| operator new(unsigned long)+0x1b < void std::vector<QQmlComponentPrivate::ConstructionState, std::allocator<QQmlComponentPrivate::ConstructionState> >::_M_realloc_append<QQmlComponentPrivate::ConstructionState>(QQmlComponentPrivate::ConstructionState&&)+0x6e8c < QQmlData::deferData(int, QQmlRefPointer<QV4::ExecutableCompilationUnit> const&, QQmlRefPointer<QQmlContextData> const&, QString const&)+0x336 | 0.000 | 0.198 |
| operator new(unsigned long)+0x1b < QObjectPrivate::connectImpl(QObject const*, int, QObject const*, void**, QtPrivate::QSlotObjectBase*, int, int const*, QMetaObject const*)+0x13d < QObject::connectImpl(QObject const*, void**, QObject const*, void**, QtPrivate::QSlotObjectBase*, Qt::ConnectionType, int const*, QMetaObject const*)+0x104 | 0.029 | 0.138 |
| operator new(unsigned long)+0x1b < operator new(unsigned long, std::nothrow_t const&)+0xc < QMetaType::create(void const*) const+0xd9 | 0.113 | -0.008 |
| operator new(unsigned long)+0x1b < QV4::Value::asArrayLength(bool*) const+0x330 < QV4::ExecutionEngine::newVariantObject(QMetaType, void const*)+0xda | 0.111 | -0.007 |


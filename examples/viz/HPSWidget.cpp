#include "HPSWidget.h"

#if _MSC_VER
#define snprintf _snprintf
#endif

#if defined _MSC_VER && defined USING_PUBLISH
#include "sprk_publish.h"
#endif

ts3d::HPSWidget::HPSWidget(QWidget *parent )
: QWidget(parent) {
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setBackgroundRole(QPalette::NoRole);
    
    setFocusPolicy(Qt::StrongFocus);
    
    HPS::ApplicationWindowOptionsKit windowOpts;
    windowOpts.SetAntiAliasCapable(true);
    
    _canvas = HPS::Factory::CreateCanvas(static_cast<HPS::WindowHandle>(winId()), "HPS Qt Sandbox", windowOpts);
    Q_ASSERT(sizeof(HPS::WindowHandle) >= sizeof(WId));
    
    _view = HPS::Factory::CreateView();
    _canvas.AttachViewAsLayout(_view);
    
    setupSceneDefaults();
}

ts3d::HPSWidget::~HPSWidget() {
    _canvas.Delete();
}

HPS::Canvas ts3d::HPSWidget::getCanvas( void ) {
    return _canvas;
}

HPS::View ts3d::HPSWidget::getView( void ) {
    return _view;
}

void ts3d::HPSWidget::setupSceneDefaults() {
    _view.GetOperatorControl()
    .Push(new HPS::MouseWheelOperator(), HPS::Operator::Priority::Low)
    .Push(new HPS::ZoomOperator(HPS::MouseButtons::ButtonMiddle()))
    .Push(new HPS::PanOperator(HPS::MouseButtons::ButtonRight()))
    .Push(new HPS::OrbitOperator(HPS::MouseButtons::ButtonLeft()));
    
    //HPS::Database::GetEventDispatcher().Subscribe(errorHandler, HPS::Object::ClassID<HPS::ErrorEvent>());
    //HPS::Database::GetEventDispatcher().Subscribe(warningHandler, HPS::Object::ClassID<HPS::WarningEvent>());
}

void ts3d::HPSWidget::paintEvent(QPaintEvent *)
{
    _canvas.GetWindowKey().Update( HPS::Window::UpdateType::Refresh );
}

void ts3d::HPSWidget::resizeEvent(QResizeEvent *)
{
    _canvas.GetWindowKey().Update(HPS::Window::UpdateType::Refresh);
}

void ts3d::HPSWidget::mousePressEvent(QMouseEvent * event)
{
    _canvas.GetWindowKey().GetEventDispatcher().InjectEvent(buildMouseEvent(event, HPS::MouseEvent::Action::ButtonDown, 1));
}

void ts3d::HPSWidget::mouseDoubleClickEvent(QMouseEvent * event)
{
    _canvas.GetWindowKey().GetEventDispatcher().InjectEvent(buildMouseEvent(event, HPS::MouseEvent::Action::ButtonDown, 2));
}

void ts3d::HPSWidget::mouseReleaseEvent(QMouseEvent * event)
{
    _canvas.GetWindowKey().GetEventDispatcher().InjectEvent(buildMouseEvent(event, HPS::MouseEvent::Action::ButtonUp, 0));
}

void ts3d::HPSWidget::mouseMoveEvent(QMouseEvent * event)
{
    _canvas.GetWindowKey().GetEventDispatcher().InjectEvent(buildMouseEvent(event, HPS::MouseEvent::Action::Move, 0));
}

void ts3d::HPSWidget::keyPressEvent(QKeyEvent * event)
{
    _canvas.GetWindowKey().GetEventDispatcher().InjectEvent(buildKeyboardEvent(event, HPS::KeyboardEvent::Action::KeyDown));
}

void ts3d::HPSWidget::keyReleaseEvent(QKeyEvent * event)
{
    _canvas.GetWindowKey().GetEventDispatcher().InjectEvent(buildKeyboardEvent(event, HPS::KeyboardEvent::Action::KeyUp));
}

void ts3d::HPSWidget::wheelEvent(QWheelEvent * event)
{
    HPS::Point pos(event->x(), event->y(), 0);
    _canvas.GetWindowKey().ConvertCoordinate(HPS::Coordinate::Space::Pixel, pos, HPS::Coordinate::Space::Window, pos);
    
    HPS::MouseEvent out_event;
    out_event.CurrentAction = HPS::MouseEvent::Action::Scroll;
    out_event.Location = pos;
    
    //NOTE: the delta() function is obsolete as of QT5.
    //Try to replace it with pixelDelta or angleDelta
    out_event.WheelDelta = event->delta();
    
    getModifierKeys(&out_event);
    
    _canvas.GetWindowKey().GetEventDispatcher().InjectEvent(out_event);
}


HPS::MouseEvent ts3d::HPSWidget::buildMouseEvent(QMouseEvent * in_event, HPS::MouseEvent::Action action, size_t click_count)
{
    auto scaleFactor = this->devicePixelRatio();
    
    HPS::MouseEvent out_event;
    
    out_event.CurrentAction = action;
    out_event.ClickCount = click_count;
    
    if (in_event->button() == Qt::MouseButton::LeftButton)
        out_event.CurrentButton = HPS::MouseButtons::ButtonLeft();
    else if (in_event->button() == Qt::MouseButton::RightButton)
        out_event.CurrentButton = HPS::MouseButtons::ButtonRight();
    else if (in_event->button() == Qt::MouseButton::MiddleButton)
        out_event.CurrentButton = HPS::MouseButtons::ButtonMiddle();
    
    HPS::Point pos(in_event->x() * scaleFactor, in_event->y() * scaleFactor, 0);
    _canvas.GetWindowKey().ConvertCoordinate(HPS::Coordinate::Space::Pixel, pos, HPS::Coordinate::Space::Window, pos);
    out_event.Location = pos;
    
    getModifierKeys(&out_event);
    
    return out_event;
}

HPS::KeyboardEvent ts3d::HPSWidget::buildKeyboardEvent(QKeyEvent *in_event, HPS::KeyboardEvent::Action action)
{
    HPS::KeyboardEvent out_event;
    out_event.CurrentAction = action;
    
    HPS::KeyboardCodeArray code;
    code.push_back((HPS::KeyboardCode)in_event->key());
    out_event.KeyboardCodes = code;
    
    getModifierKeys(&out_event);
    
    return out_event;
    
}

void ts3d::HPSWidget::getModifierKeys(HPS::InputEvent * event)
{
    Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
    if (modifiers.testFlag(Qt::KeyboardModifier::ShiftModifier))
        event->ModifierKeyState.Shift(true);
    if (modifiers.testFlag(Qt::KeyboardModifier::ControlModifier))
        event->ModifierKeyState.Control(true);
}

//
//void ts3d::HPSWidget::onFileNew()
//{
//    // Restore scene defaults if we already initialized our _canvas
//    if (_canvas.Type() != HPS::Type::None)
//    {
//        setupSceneDefaults();
//        _canvas.Update();
//    }
//}
//
//HPS::Stream::ImportNotifier ts3d::HPSWidget::importHSFFile(QString filename, QProgressDialog * progressDlg, bool & success)
//{
//    HPS::IOResult status = HPS::IOResult::Failure;
//    HPS::Stream::ImportNotifier notifier;
//    try
//    {
//        HPS::Stream::ImportOptionsKit ioKit;
//        ioKit.SetSegment(model.GetSegmentKey());
//        ioKit.SetAlternateRoot(model.GetLibraryKey());
//        ioKit.SetPortfolio(model.GetPortfolioKey());
//        
//        notifier = HPS::Stream::File::Import(filename.toUtf8(), ioKit);
//        float percent_complete = 0;
//        status = notifier.Status(percent_complete);
//        while (status == HPS::IOResult::InProgress)
//        {
//            if (progressDlg->wasCanceled())
//            {
//                notifier.Cancel();
//                progressDlg->setValue(0);
//                success = false;
//                return notifier;
//            }
//            progressDlg->setValue((int)(percent_complete * 100));
//            status = notifier.Status(percent_complete);
//        }
//    }
//    catch (HPS::IOException const & ex)
//    {
//        status = ex.result;
//    }
//    
//    if (status == HPS::IOResult::Failure)
//    {
//        success = false;
//        QMessageBox msgBox;
//        char error_message[1024];
//        sprintf(error_message, "Error loading file %s", filename.toUtf8().constData());
//        msgBox.setText(error_message);
//        msgBox.exec();
//    }
//    
//    else if (status == HPS::IOResult::FileNotFound)
//    {
//        success = false;
//        QMessageBox msgBox;
//        char error_message[1024];
//        sprintf(error_message, "Could not find file %s", filename.toUtf8().constData());
//        msgBox.setText(error_message);
//        msgBox.exec();
//    }
//    
//    notifier.Wait();
//    return notifier;
//}
//
//void ts3d::HPSWidget::importSTLFile(QString filename, QProgressDialog * progressDlg, bool & success)
//{
//    HPS::IOResult status = HPS::IOResult::Failure;
//    HPS::STL::ImportNotifier notifier;
//    try
//    {
//        HPS::STL::ImportOptionsKit ioKit = HPS::STL::ImportOptionsKit::GetDefault();
//        ioKit.SetSegment(model.GetSegmentKey());
//        
//        notifier = HPS::STL::File::Import(filename.toUtf8(), ioKit);
//        float percent_complete = 0;
//        status = notifier.Status(percent_complete);
//        while (status == HPS::IOResult::InProgress)
//        {
//            if (progressDlg->wasCanceled())
//            {
//                notifier.Cancel();
//                progressDlg->setValue(0);
//                success = false;
//                return;
//            }
//            progressDlg->setValue((int)(percent_complete * 100));
//            status = notifier.Status(percent_complete);
//        }
//    }
//    catch (HPS::IOException const & ex)
//    {
//        success = false;
//        status = ex.result;
//    }
//    
//    if (status == HPS::IOResult::Failure)
//    {
//        QMessageBox msgBox;
//        char error_message[1024];
//        sprintf(error_message, "Error loading file %s", filename.toUtf8().constData());
//        msgBox.setText(error_message);
//        msgBox.exec();
//    }
//    
//    else if (status == HPS::IOResult::FileNotFound)
//    {
//        QMessageBox msgBox;
//        char error_message[1024];
//        sprintf(error_message, "Could not find file %s", filename.toUtf8().constData());
//        msgBox.setText(error_message);
//        msgBox.exec();
//    }
//    
//    notifier.Wait();
//}
//
//void ts3d::HPSWidget::importOBJFile(QString filename, QProgressDialog * progressDlg, bool & success)
//{
//    HPS::IOResult status = HPS::IOResult::Failure;
//    HPS::OBJ::ImportNotifier notifier;
//    try
//    {
//        HPS::OBJ::ImportOptionsKit ioKit;
//        ioKit.SetSegment(model.GetSegmentKey());
//        ioKit.SetPortfolio(model.GetPortfolioKey());
//        
//        notifier = HPS::OBJ::File::Import(filename.toUtf8(), ioKit);
//        float percent_complete = 0;
//        status = notifier.Status(percent_complete);
//        while (status == HPS::IOResult::InProgress)
//        {
//            if (progressDlg->wasCanceled())
//            {
//                notifier.Cancel();
//                progressDlg->setValue(0);
//                success = false;
//                return;
//            }
//            progressDlg->setValue((int)(percent_complete * 100));
//            status = notifier.Status(percent_complete);
//        }
//    }
//    catch (HPS::IOException const & ex)
//    {
//        status = ex.result;
//        success = false;
//    }
//    
//    if (status == HPS::IOResult::Failure)
//    {
//        QMessageBox msgBox;
//        char error_message[1024];
//        sprintf(error_message, "Error loading file %s", filename.toUtf8().constData());
//        msgBox.setText(error_message);
//        msgBox.exec();
//    }
//    
//    else if (status == HPS::IOResult::FileNotFound)
//    {
//        QMessageBox msgBox;
//        char error_message[1024];
//        sprintf(error_message, "Could not find file %s", filename.toUtf8().constData());
//        msgBox.setText(error_message);
//        msgBox.exec();
//    }
//    
//    notifier.Wait();
//}
//
//#ifdef USING_EXCHANGE
//void ts3d::HPSWidget::importExchangeFile(QString filename, QProgressDialog * /*progressDlg*/, bool & success)
//{
//    HPS::IOResult status = HPS::IOResult::Failure;
//    HPS::Exchange::ImportNotifier notifier;
//    try
//    {
//        HPS::Exchange::ImportOptionsKit ioKit;
//        ioKit.SetBRepMode(HPS::Exchange::BRepMode::BRepAndTessellation);
//        
//        notifier = HPS::Exchange::File::Import(filename.toUtf8(), ioKit);
//        
//        ExchangeImportDialog dlg(notifier, this);
//        int index = filename.lastIndexOf(QString("/"));
//        if (index != -1)
//            dlg.setWindowTitle(filename.right(filename.size() - index));
//        else
//        {
//            int index = filename.lastIndexOf(QString("\\"));
//            if (index != -1)
//                dlg.setWindowTitle(filename.right(filename.size() - index));
//            else
//                dlg.setWindowTitle(filename);
//        }
//        dlg.exec();
//        
//        status = notifier.Status();
//        success = dlg.WasImportSuccessful();
//    }
//    catch (HPS::IOException const & ex)
//    {
//        status = ex.result;
//        success = false;
//    }
//    
//    if (status == HPS::IOResult::Failure)
//    {
//        QMessageBox msgBox;
//        char error_message[1024];
//        sprintf(error_message, "Error loading file %s", filename.toUtf8().constData());
//        msgBox.setText(error_message);
//        msgBox.exec();
//    }
//    
//    else if (status == HPS::IOResult::FileNotFound)
//    {
//        QMessageBox msgBox;
//        char error_message[1024];
//        sprintf(error_message, "Could not find file %s", filename.toUtf8().constData());
//        msgBox.setText(error_message);
//        msgBox.exec();
//    }
//    
//    cad_model = notifier.GetCADModel();
//}
//#endif

//void ts3d::HPSWidget::AttachView(HPS::View const & in_view)
//{
//    HPS::View old_view = _canvas.GetFrontView();
//    
//    _canvas.AttachViewAsLayout(in_view);
//    
//    HPS::OperatorPtrArray operators;
//    auto oldViewOperatorCtrl = old_view.GetOperatorControl();
//    auto newViewOperatorCtrl = in_view.GetOperatorControl();
//    oldViewOperatorCtrl.Show(HPS::Operator::Priority::Low, operators);
//    newViewOperatorCtrl.Set(operators, HPS::Operator::Priority::Low);
//    oldViewOperatorCtrl.Show(HPS::Operator::Priority::Default, operators);
//    newViewOperatorCtrl.Set(operators, HPS::Operator::Priority::Default);
//    oldViewOperatorCtrl.Show(HPS::Operator::Priority::High, operators);
//    newViewOperatorCtrl.Set(operators, HPS::Operator::Priority::High);
//    
//    HPS::DistantLightKit light;
//    light.SetDirection(HPS::Vector(1, 0, -1.5f));
//    light.SetCameraRelative(true);
//    
//    // Delete previous light before inserting new one
//    if (_mainDistantLight.Type() != HPS::Type::None)
//        _mainDistantLight.Delete();
//    _mainDistantLight = _canvas.GetFrontView().GetSegmentKey().InsertDistantLight(light);
//    
//    old_view.Delete();
//    
//    _view = in_view;
//}

//void ts3d::HPSWidget::onFileOpen(QString filename)
//{
//#ifdef USING_EXCHANGE
//    if (filename.isEmpty())
//        filename = QFileDialog::getOpenFileName(this, tr("Open File"), ".",
//                                                tr("All Supported Files (*.3ds *.3dxml *.sat *.sab *_pd *.model *.dlv *.exp *.session *.CATPart *.CATProduct *.CATShape *.CATDrawing"
//                                                   " *.cgr *.dae *.prt *.prt.* *.neu *.neu.* *.asm *.asm.* *.xas *.xpr *.arc *.unv *.mf1 *.prt *.pkg *.ifc *.ifczip *.igs *.iges *.ipt *.iam"
//                                                   " *.jt *.kmz *.prt *.pdf *.prc *.x_t *.xmt *.x_b *.xmt_txt *.3dm *.stp *.step *.stpz *.stp.z *.stl *.par *.asm *.pwd *.psm"
//                                                   " *.sldprt *.sldasm *.sldfpp *.asm *.u3d *.vda *.wrl *.vml *.obj *.xv3 *.xv0 *.hsf);;"
//                                                   "HSF files (*.hsf);;"
//                                                   "StereoLithography File (*.stl);;"
//                                                   "Wavefront File (*.obj);;"
//                                                   "3D Studio Files (*.3ds);;"
//                                                   "3DXML Files (*.3dxml);;"
//                                                   "ACIS SAT Files (*.sat *.sab);;"
//                                                   "CADDS Files (*_pd);;"
//                                                   "CATIA V4 Files (*.model *.dlv *.exp *.session);;"
//                                                   "CATIA V5 Files (*.CATPart *.CATProduct *.CATShape *.CATDrawing);;"
//                                                   "CGR Files (*.cgr);;"
//                                                   "Collada Files (*.dae);;"
//                                                   "Creo (ProE) Files (*.prt *.prt.* *.neu *.neu.* *.asm *.asm.* *.xas *.xpr);;"
//                                                   "I-DEAS Files (*.arc *.unv *.mf1 *.prt *.pkg);;"
//                                                   "IFC Files (*.ifc *.ifczip);;"
//                                                   "IGES Files (*.igs *.iges);;"
//                                                   "Inventor Files (*.ipt *.iam);;"
//                                                   "JT Files (*.jt);;"
//                                                   "KMZ Files (*.kmz);;"
//                                                   "NX (Unigraphics) Files (*.prt);;"
//                                                   "PDF Files (*.pdf);;"
//                                                   "PRC Files (*.prc);;"
//                                                   "Parasolid Files (*.x_t *.xmt *.x_b *.xmt_txt);;"
//                                                   "Rhino Files (*.3dm);;"
//                                                   "STEP Files (*.stp *.step *.stpz *.stp.z);;"
//                                                   "SolidEdge Files (*.par *.asm *.pwd *.psm);;"
//                                                   "SolidWorks Files (*.sldprt *.sldasm *.sldfpp *.asm);;"
//                                                   "Universal 3D Files (*.u3d);;"
//                                                   "VDA Files (*.vda);;"
//                                                   "VRML Files (*.wrl *.vrml);;"
//                                                   "WaveFront Files (*.obj);;"
//                                                   "XVL Files (*.xv3 *.xv0);;"
//                                                   "All Files (*.*)")
//                                                , 0);
//#else
//    if (filename.isEmpty())
//        filename = QFileDialog::getOpenFileName(this, tr("Open File"), ".",
//                                                tr("HOOPS Stream Files (*.hsf);;StereoLithography File (*.stl);;Wavefront File (*.obj)"), 0);
//#endif
//    
//    if (filename.size() > 0)
//    {
//        QProgressDialog * progressDlg;
//        progressDlg = new QProgressDialog("Loading File...", "Cancel", 0, 100, parentWidget());
//#ifndef USING_EXCHANGE
//        progressDlg->setWindowModality(Qt::WindowModal);
//        progressDlg->setValue(0);
//        progressDlg->show();
//#endif
//        
//        
//        // Delete our model if we have one already
//        if (model.Type() != HPS::Type::None)
//            model.Delete();
//        
//        // Create a new model for our _view to attach to
//        model = HPS::Factory::CreateModel();
//        
//        bool success = true;
//        if (filename.endsWith(".hsf"))
//        {
//            HPS::Stream::ImportNotifier stream_notifier = importHSFFile(filename, progressDlg, success);
//            if (success)
//            {
//                _view.AttachModel(model);
//                
//                HPS::CameraKit	defaultCamera;
//                if (stream_notifier.GetResults().ShowDefaultCamera(defaultCamera))
//                    _view.GetSegmentKey().SetCamera(defaultCamera);
//            }
//        }
//        else if (filename.endsWith(".stl"))
//        {
//            importSTLFile(filename, progressDlg, success);
//            if (success)
//            {
//                _view.AttachModel(model);
//                _canvas.GetFrontView().FitWorld();
//            }
//        }
//        else if (filename.endsWith(".obj"))
//        {
//            importOBJFile(filename, progressDlg, success);
//            if (success)
//            {
//                _view.AttachModel(model);
//                _canvas.GetFrontView().FitWorld();
//            }
//        }
//#ifdef USING_EXCHANGE
//        else
//        {
//            model.Delete();
//            cad_model.Delete();
//            importExchangeFile(filename, progressDlg, success);
//        }
//#endif
//        
//        if (success)
//        {
//            HPS::DistantLightKit light;
//            light.SetDirection(HPS::Vector(1, 0, -1.5f));
//            light.SetCameraRelative(true);
//            
//            // Delete previous light before inserting new one
//            if (mainDistantLight.Type() != HPS::Type::None)
//                mainDistantLight.Delete();
//            mainDistantLight = _canvas.GetFrontView().GetSegmentKey().InsertDistantLight(light);
//            
//            _canvas.UpdateWithNotifier(HPS::Window::UpdateType::Exhaustive).Wait();
//            progressDlg->setValue(100);
//            delete progressDlg;
//            
//            HPSMainWindow * mw = (HPSMainWindow *)parentWidget();
//            mw->setCurrentFile(filename);
//        }
//        else
//        {
//            delete progressDlg;
//            model.GetSegmentKey().Flush();
//        }
//    }
//}
//
//void ts3d::HPSWidget::onFileSaveAs()
//{
//    QString selected_filter;
//#if defined _MSC_VER && defined USING_PUBLISH
//    QString filename = QFileDialog::getSaveFileName(this, "Save As...", ".",
//                                                    "HOOPS Stream File (*.hsf);;PDF (*.pdf);;Postscript File (*.ps);;JPEG Image File (*.jpeg);;PNG Image File (*.png);;3D PDF (*.pdf)", &selected_filter);
//#else
//    QString filename = QFileDialog::getSaveFileName(this, "Save As...", ".",
//                                                    "HOOPS Stream File (*.hsf);;PDF (*.pdf);;Postscript File (*.ps);;JPEG Image File (*.jpeg);;PNG Image File (*.png)", &selected_filter);
//#endif
//    
//    if (filename.size() > 0)
//    {
//        if (QString::compare(selected_filter, QString("HOOPS Stream File (*.hsf)"), Qt::CaseInsensitive) == 0)
//        {
//            QProgressDialog * progressDlg;
//            progressDlg = new QProgressDialog("Saving File...", "Cancel", 0, 100, parentWidget());
//            progressDlg->setWindowModality(Qt::WindowModal);
//            progressDlg->show();
//            progressDlg->setValue(0);
//            
//            HPS::Stream::ExportOptionsKit eok;
//            HPS::SegmentKey exportFromHere;
//            
//            HPS::Model model = _canvas.GetFrontView().GetAttachedModel();
//            if (model.Type() == HPS::Type::None)
//                exportFromHere = _canvas.GetFrontView().GetSegmentKey();
//            else
//                exportFromHere = model.GetSegmentKey();
//            
//            HPS::Stream::ExportNotifier notifier;
//            HPS::IOResult status;
//            try
//            {
//                notifier = HPS::Stream::File::Export(filename.toUtf8(), exportFromHere, eok);
//                float percent_complete = 0;
//                status = notifier.Status(percent_complete);
//                while (status == HPS::IOResult::InProgress)
//                {
//                    if (progressDlg->wasCanceled())
//                    {
//                        notifier.Cancel();
//                        progressDlg->setValue(0);
//                        break;
//                    }
//                    progressDlg->setValue((int)(percent_complete * 100));
//                    status = notifier.Status(percent_complete);
//                }
//            }
//            catch (HPS::IOException const & e)
//            {
//                QMessageBox msgBox;
//                char error_message[1024];
//                snprintf(error_message, 1024, "HPS::Stream::File::Export threw an exception: %s", e.what());
//                msgBox.setText(error_message);
//                msgBox.exec();
//            }
//            
//        }
//        else if (QString::compare(selected_filter, QString("PDF (*.pdf)"), Qt::CaseInsensitive) == 0)
//        {
//            try
//            {
//                HPS::Hardcopy::File::Export(filename.toUtf8(), HPS::Hardcopy::File::Driver::PDF,
//                                            _canvas.GetWindowKey(), HPS::Hardcopy::File::ExportOptionsKit::GetDefault());
//            }
//            catch (HPS::IOException const & e)
//            {
//                QMessageBox msgBox;
//                char error_message[1024];
//                snprintf(error_message, 1024, "HPS::Hardcopy::File::Export threw an exception: %s", e.what());
//                msgBox.setText(error_message);
//                msgBox.exec();
//            }
//        }
//        else if (QString::compare(selected_filter, QString("Postscript File (*.ps)"), Qt::CaseInsensitive) == 0)
//        {
//            try
//            {
//                HPS::Hardcopy::File::Export(filename.toUtf8(), HPS::Hardcopy::File::Driver::Postscript,
//                                            _canvas.GetWindowKey(), HPS::Hardcopy::File::ExportOptionsKit::GetDefault());
//            }
//            catch (HPS::IOException const & e)
//            {
//                QMessageBox msgBox;
//                char error_message[1024];
//                snprintf(error_message, 1024, "HPS::Hardcopy::File::Export threw an exception: %s", e.what());
//                msgBox.setText(error_message);
//                msgBox.exec();
//            }
//        }
//        else if (QString::compare(selected_filter, QString("JPEG Image File (*.jpeg)"), Qt::CaseInsensitive) == 0)
//        {
//            HPS::Image::ExportOptionsKit eok;
//            eok.SetFormat(HPS::Image::Format::Jpeg);
//            
//            try { HPS::Image::File::Export(filename.toUtf8(), _canvas.GetWindowKey(), eok); }
//            catch (HPS::IOException const & e)
//            {
//                QMessageBox msgBox;
//                char error_message[1024];
//                snprintf(error_message, 1024, "HPS::Image::File::Export threw an exception: %s", e.what());
//                msgBox.setText(error_message);
//                msgBox.exec();
//            }
//        }
//        else if (QString::compare(selected_filter, QString("PNG Image File (*.png)"), Qt::CaseInsensitive) == 0)
//        {
//            try { HPS::Image::File::Export(filename.toUtf8(), _canvas.GetWindowKey(), HPS::Image::ExportOptionsKit::GetDefault()); }
//            catch (HPS::IOException const & e)
//            {
//                QMessageBox msgBox;
//                char error_message[1024];
//                snprintf(error_message, 1024, "HPS::Image::File::Export threw an exception: %s", e.what());
//                msgBox.setText(error_message);
//                msgBox.exec();
//            }
//        }
//#if defined _MSC_VER && defined USING_PUBLISH
//        else if (QString::compare(selected_filter, QString("3D PDF (*.pdf)"), Qt::CaseInsensitive) == 0)
//        {
//            try 
//            {
//                HPS::SprocketPath sprocket_path(*getCanvas(), getCanvas()->GetAttachedLayout(), getCanvas()->GetFrontView(), getCanvas()->GetFrontView().GetAttachedModel());
//                HPS::Publish::ExportOptionsKit export_kit;
//				HPS::Publish::File::ExportPDF(sprocket_path.GetKeyPath(), filename.toUtf8(), export_kit); 
//            }
//            catch (HPS::IOException const & e)
//            {
//                QMessageBox msgBox;
//                char error_message[1024];
//                snprintf(error_message, 1024, "HPS::Publish::Export threw an exception: %s", e.what());
//                msgBox.setText(error_message);
//                msgBox.exec();
//            }
//        }
//#endif
//    }
//}
//
//void ts3d::HPSWidget::onOperatorOrbit() 
//{
//    _canvas.GetFrontView().GetOperatorControl().Pop();
//    _canvas.GetFrontView().GetOperatorControl().Push(new HPS::OrbitOperator(HPS::MouseButtons::ButtonLeft()));
//}
//
//void ts3d::HPSWidget::onOperatorPan() 
//{
//    _canvas.GetFrontView().GetOperatorControl().Pop();
//    _canvas.GetFrontView().GetOperatorControl().Push(new HPS::PanOperator(HPS::MouseButtons::ButtonLeft()));
//}
//
//void ts3d::HPSWidget::onOperatorZoomArea()
//{
//    _canvas.GetFrontView().GetOperatorControl().Pop();
//    _canvas.GetFrontView().GetOperatorControl().Push(new HPS::ZoomBoxOperator(HPS::MouseButtons::ButtonLeft()));
//}
//
//void ts3d::HPSWidget::onOperatorFly() 
//{
//    _canvas.GetFrontView().GetOperatorControl().Pop();
//    _canvas.GetFrontView().GetOperatorControl().Push(new HPS::FlyOperator());
//}
//
//void ts3d::HPSWidget::onOperatorZoomFit() 
//{
//    _canvas.GetFrontView().FitWorld().Update();
//}
//
//void ts3d::HPSWidget::onOperatorPoint() 
//{
//    _canvas.GetFrontView().GetOperatorControl().Pop();
//    _canvas.GetFrontView().GetOperatorControl().Push(new HPS::HighlightOperator(HPS::MouseButtons::ButtonLeft()));
//}
//
//void ts3d::HPSWidget::onOperatorArea() 
//{
//    _canvas.GetFrontView().GetOperatorControl().Pop();
//    _canvas.GetFrontView().GetOperatorControl().Push(new HPS::HighlightAreaOperator(HPS::MouseButtons::ButtonLeft()));
//}
//
//void ts3d::HPSWidget::onModeSimpleShadow() 
//{
//    // Toggle state and bail early if we're disabling
//    enableSimpleShadows = !enableSimpleShadows;
//    if (enableSimpleShadows == false)
//    {
//        _canvas.GetFrontView().GetSegmentKey().GetVisualEffectsControl().SetSimpleShadow(false);
//        _canvas.Update();
//        return;
//    }
//    
//    updatePlanes();
//}
//
//
//void ts3d::HPSWidget::onModeFrameRate()
//{
//    const float					frameRate = 20.0f;
//    
//    // Toggle frame rate and set.  Note that 0 disables frame rate.
//    enableFrameRate = !enableFrameRate;
//    if (enableFrameRate)
//    {
//        _canvas.SetFrameRate(frameRate);
//        if (!smoothRendering)
//        {
//            smoothRendering = true;
//            HPSMainWindow * mw = (HPSMainWindow *)parentWidget();
//            mw->toolbarSmooth->setChecked(true);
//            _canvas.GetFrontView().SetRenderingMode(HPS::Rendering::Mode::Phong);
//        }
//    }
//    else
//        _canvas.SetFrameRate(0);
//    
//    _canvas.Update();
//}
//
//void ts3d::HPSWidget::onModeSmooth() 
//{
//    if (!smoothRendering)
//    {
//        _canvas.GetFrontView().SetRenderingMode(HPS::Rendering::Mode::Phong);
//        _canvas.Update();
//        smoothRendering = true;
//    }
//}
//
//void ts3d::HPSWidget::onModeHiddenLine() 
//{
//    if (smoothRendering)
//    {
//        _canvas.GetFrontView().SetRenderingMode(HPS::Rendering::Mode::FastHiddenLine);
//        _canvas.SetFrameRate(0);
//        _canvas.Update();
//        smoothRendering = false;
//    }
//}

//void ts3d::HPSWidget::updatePlanes()
//{
//    if(enableSimpleShadows)
//    {
//        _canvas.GetFrontView().SetSimpleShadow(true);
//        
//        const float					opacity = 0.3f;
//        const unsigned int			resolution = 512;
//        const unsigned int			blurring = 20;
//        
//        HPS::SegmentKey _viewSegment = _canvas.GetFrontView().GetSegmentKey();
//        
//        // Set opacity in simple shadow color
//        HPS::RGBAColor color(0.4f, 0.4f, 0.4f, opacity);
//        if (_viewSegment.GetVisualEffectsControl().ShowSimpleShadowColor(color))
//            color.alpha = opacity;
//        
//        _viewSegment.GetVisualEffectsControl()
//        .SetSimpleShadow(enableSimpleShadows, resolution, blurring)
//        .SetSimpleShadowColor(color);
//        _canvas.Update();
//    }
//}

void ts3d::HPSWidget::focusOutEvent(QFocusEvent *)
{
    _canvas.GetWindowKey().GetEventDispatcher().InjectEvent(HPS::FocusLostEvent());
}

//void ts3d::HPSWidget::onUserCode1() 
//{
//    // Toggle display of resource monitor using the DebuggingControl
//    displayResourceMonitor = !displayResourceMonitor;
//    _canvas.GetWindowKey().GetDebuggingControl().SetResourceMonitor(displayResourceMonitor);
//    
//    _canvas.Update();
//}
//
//void ts3d::HPSWidget::onUserCode2() 
//{
//    // TODO: Add your code here
//}
//
//void ts3d::HPSWidget::onUserCode3()
//{
//    // TODO: Add your code here
//}
//
//void ts3d::HPSWidget::onUserCode4() 
//{
//    // TODO: Add your code here
//}

#include <widget/widget_shared.h>
#include <widget/xmessagebox.h>
#include <widget/widget_shared_global.h>

void logAndShowMessage(const std::exception_ptr& ptr) {
    try {
        std::rethrow_exception(ptr);
    }
    catch (const PlatformException& e) {
        XAMP_LOG_DEBUG("{}", e.GetErrorMessage());
        XMessageBox::showError(qTEXT(e.GetErrorMessage()));
    }
    catch (const Exception& e) {
        XAMP_LOG_DEBUG("{}", e.GetErrorMessage());
        XMessageBox::showError(qTEXT(e.GetErrorMessage()));
    }
    catch (const std::exception& e) {
        XAMP_LOG_DEBUG("{}", String::LocaleStringToUTF8(e.what()));
        XMessageBox::showError(QString::fromStdString(String::LocaleStringToUTF8(e.what())));
    }
    catch (...) {
        XAMP_LOG_DEBUG("Unknown error");
        XMessageBox::showError(qTR("Unknown error"));
    }
}

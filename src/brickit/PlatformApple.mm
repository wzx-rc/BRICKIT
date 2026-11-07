// BrickIt - the system dialogs of Platform.h on the iPad: alerts, the document picker to open a file, and saving into
// the Documents folder of the app (visible in the Files app: "Sur mon iPad > BrickIt").
#include "Platform.h"
#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

@interface BKPickerDelegate : NSObject <UIDocumentPickerDelegate>
@property(nonatomic) std::function<void(const std::wstring&)> done;
@end

@implementation BKPickerDelegate
- (void)documentPicker:(UIDocumentPickerViewController*)c didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls {
    if (urls.count == 0 || !self.done) return;
    // (asCopy: the file is a copy in the app's inbox, readable without security scope)
    self.done(brickit::plat::fromUtf8(urls[0].path.UTF8String));
}
@end

namespace brickit::plat {

static BKPickerDelegate* g_picker = nil;   // the picker keeps a weak reference to its delegate

static UIViewController* topController() {
    UIWindow* win = nil;
    for (UIScene* s in UIApplication.sharedApplication.connectedScenes)
        if ([s isKindOfClass:[UIWindowScene class]])
            for (UIWindow* w in ((UIWindowScene*)s).windows)
                if (w.isKeyWindow) win = w;
    UIViewController* c = win.rootViewController;
    while (c.presentedViewController) c = c.presentedViewController;
    return c;
}

static NSString* ns(const std::wstring& w) { return [NSString stringWithUTF8String:toUtf8(w).c_str()]; }

void setWindow(void*) {}

void errorBox(const std::string& title, const std::string& text) {
    UIAlertController* a = [UIAlertController alertControllerWithTitle:[NSString stringWithUTF8String:title.c_str()]
                                                               message:[NSString stringWithUTF8String:text.c_str()]
                                                        preferredStyle:UIAlertControllerStyleAlert];
    [a addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:nil]];
    [topController() presentViewController:a animated:YES completion:nil];
}

void confirm(const std::wstring& title, const std::wstring& text, std::function<void()> onOk) {
    UIAlertController* a = [UIAlertController alertControllerWithTitle:ns(title) message:ns(text)
                                                        preferredStyle:UIAlertControllerStyleAlert];
    [a addAction:[UIAlertAction actionWithTitle:@"Annuler" style:UIAlertActionStyleCancel handler:nil]];
    [a addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction*) {
           if (onOk) onOk();
       }]];
    [topController() presentViewController:a animated:YES completion:nil];
}

// the extensions of a Windows filter ("Name\0*.a;*.b\0..."): content types for the picker (any file when "*.*")
static NSArray<UTType*>* typesOf(const wchar_t* filter) {
    NSMutableArray<UTType*>* types = [NSMutableArray array];
    bool any = !filter;
    for (const wchar_t* p = filter; p && *p; p += wcslen(p) + 1) {
        const wchar_t* pattern = p + wcslen(p) + 1;   // name, then its patterns
        if (!*pattern) break;
        std::wstring pats = pattern;
        size_t s = 0;
        while (s <= pats.size()) {
            size_t e = pats.find(L';', s);
            if (e == std::wstring::npos) e = pats.size();
            std::wstring one = pats.substr(s, e - s);
            if (one == L"*.*" || one == L"*") any = true;
            else if (one.size() > 2) {
                const std::wstring ext = one.substr(one.find_last_of(L'.') + 1);
                UTType* t = [UTType typeWithFilenameExtension:ns(ext)];
                if (t) [types addObject:t];
            }
            s = e + 1;
        }
        p = pattern;
    }
    if (any || types.count == 0) return @[ UTTypeItem ];
    [types addObject:UTTypeData];   // multi-dot extensions (.v3d.json) and unknown ones
    return types;
}

void openFileDialog(const wchar_t* filter, const wchar_t*, std::function<void(const std::wstring&)> done, const std::wstring&) {
    UIDocumentPickerViewController* p = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:typesOf(filter) asCopy:YES];
    g_picker = [BKPickerDelegate new];
    g_picker.done = done;
    p.delegate = g_picker;
    [topController() presentViewController:p animated:YES completion:nil];
}

// Saving: no dialog, the file goes to Documents with the suggested name (the Files app shows it).
void saveFileDialog(const wchar_t*, const wchar_t* defExt, const std::wstring& suggested, std::function<void(const std::wstring&)> done) {
    NSString* docs = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
    std::wstring name = suggested.substr(suggested.find_last_of(L"\\/") + 1);
    if (name.empty()) name = std::wstring(L"BrickIt") + (defExt ? std::wstring(L".") + defExt : std::wstring());
    if (done) done(fromUtf8(docs.UTF8String) + L"/" + name);
}

} // namespace brickit::plat

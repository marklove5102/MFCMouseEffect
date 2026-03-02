#include "pch.h"

#include "Platform/macos/Shell/MacosTrayMenuLocalization.h"

#if defined(__APPLE__)
#import <Foundation/Foundation.h>
#endif

namespace mousefx {

MacosTrayMenuText ResolveMacosTrayMenuText() {
#if defined(__APPLE__)
    @autoreleasepool {
      NSArray<NSString*>* languages = [NSLocale preferredLanguages];
      NSString* firstLanguage = (languages.count > 0) ? languages[0] : nil;
      if (firstLanguage != nil && [firstLanguage hasPrefix:@"zh"]) {
          MacosTrayMenuText text;
          text.settingsTitle = u8"\u8bbe\u7f6e";
          text.exitTitle = u8"\u9000\u51fa";
          return text;
      }
    }
#endif
    return {};
}

} // namespace mousefx

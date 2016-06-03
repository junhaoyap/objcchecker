#import "WMLStoresScope.h"
#import "WMLAPIClient.h"

NSString *const WMLStoresScopeAllDisplayedString = @"All Stores";
NSString *const WMLStoresScopeFollowedDisplayedString = @"Followed Stores";

WMLStoresScope const WMLStoresScopeFromDisplayedString(NSString *scopeString) {
    if ([scopeString isEqualToString:WMLStoresScopeAllDisplayedString]) {
        return WMLStoresScopeAll;
    } else if ([scopeString isEqualToString:WMLStoresScopeFollowedDisplayedString]) {
        return WMLStoresScopeFollowed;
    } else {
        NSCAssert(NO, @"Unexpected store scope: %@", scopeString);
        return WMLStoresScopeAll;
    }
}

NSString *const displayedStringFromWMLStoresScope(WMLStoresScope scope) {
    switch (scope) {
        case WMLStoresScopeAll:
            return WMLStoresScopeAllDisplayedString;
        case WMLStoresScopeFollowed:
            return WMLStoresScopeFollowedDisplayedString;
    }
}

NSArray<NSString *> *const kWMLStoresScope() {
    return @[WMLStoresScopeAllDisplayedString, WMLStoresScopeFollowedDisplayedString];
}

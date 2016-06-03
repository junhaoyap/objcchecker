//
//  WMLStoreViewModel.m
//  Wondermall
//
//  Created by David Iskhakov on 6/10/15.
//  Copyright (c) 2015 Wondermall Inc. All rights reserved.
//

#import "WMLStoreViewModel.h"
#import "WMLAPIClient.h"
#import "WMLMacros.h"
#import "WMLMall.h"
#import "WMLStore.h"
#import "VSPNavigationNode.h"
#import "WMLStoreOpenContext.h"
#import "WMLStoreCategoryTreeResponse.h"
#import "WMLStoreCategory.h"
#import "WMLStoreCategoryPath.h"
#import "WMLUserProfile.h"
#import "WMLMallCategoriesService.h"
#import "WMLStoreMallCategory.h"
#import "NSString+Additions.h"
#import "WMLFeatureFlagsStore.h"
#import "Wondermall-Swift.h"
#import "WMLGenderCategory.h"
#import "WMLStoreOpenCategory.h"
#import "WMLStorePage.h"
#import "WMLStoreCategoryPage.h"
#import "WMLStoreHomePage.h"


@interface WMLStoreViewModel()

@property (nonatomic, strong) WMLAPIClient *mallApiClient;
@property (nonatomic, strong) WMLStoreOpenContext *openContext;

@property (nonatomic, copy) NSArray *departments;
@property (nonatomic, copy) NSArray *deals;
@property (nonatomic, strong) WMLUserProfile *userProfile;
@property (nonatomic, strong) WMLMallCategoriesService *mallCategoriesService;
@property (nonatomic, getter=isLoadingStore) BOOL loadingStore;
@property (nonatomic, getter=isFilteringEnabled) BOOL filteringEnabled;
@property (nonatomic, copy) NSArray /* WMLSearchFacet */ *facets;
@property (nonatomic) BOOL shouldPresentWonderpointsOnTopBar;

@property (nonatomic, strong) RACSignal *storeDidFailLoading;
@property (nonatomic, strong) RACSignal *storeDidLoad;

@end

@implementation WMLStoreViewModel
objection_requires(SQTypedKeyPath(WMLStoreViewModel, mallApiClient),
                   SQTypedKeyPath(WMLStoreViewModel, userProfile),
                   SQTypedKeyPath(WMLStoreViewModel, mallCategoriesService))

#pragma mark - Lifetime

- (instancetype)init {
    self = [super init];
    if (!self) {
        return nil;
    }

    self.storeDidFailLoading = [RACSubject subject];
    self.storeDidLoad = [RACSubject subject];

    [self _setupBindings];

    return self;
}

#pragma mark - Public Methods

- (void)loadStateFromNavigationNode:(VSPNavigationNode *)navigationNode {
    NSString *storeId = navigationNode.parameters[@"store_id"];
    WMLStore *store = [[WMLMall currentMall] storeForStoreId:storeId];
    NSString *eventSource = navigationNode.parameters[@"event_source"];

    self.openContext = [[WMLStoreOpenContext alloc] initWithStore:store eventSource:WMLOpenStoreEventSourceFromNSString(eventSource)];

    NSString *category = navigationNode.parameters[@"category"];
    self.openContext.categoryId = category;
}

- (void)reloadStoreInfo {
    [self _loadStore:self.openContext.store];
}

- (void)openHomepage {
    if (![WMLFeatureFlagsStore sharedInstance].isStoreHomePageEnabled || self.selectedPage.pageType == WMLStorePageTypeHomepage) {
        return;
    }

    self.selectedPage = [[WMLStoreHomePage alloc] init];
}

- (WMLStoreCategory *)initialCategoryForDepartment:(WMLStoreDepartment *)department {
    if (department.allCategory) {
        return department.allCategory;
    } else {
        return department.firstCategory;
    }
}

- (NSString *)pageTypeName {
    switch (self.selectedPage.pageType) {
        case WMLStorePageTypeHomepage: {
            return kAnalyticsParameterStoreHomePage;
        }
        case WMLStorePageTypeCategory: {
            return ((WMLStoreCategoryPage *)self.selectedPage).category.typeName;
        }
    }
}

#pragma mark - Private Methods

- (void)_setupBindings {
    [self rac_liftSelector:@selector(_openContextDidChange:) withSignals:[[RACObserve(self, openContext) skip:1] distinctUntilChanged], nil];

    RACSignal *selectedPageSignal = RACObserve(self, selectedPage);

    RAC(self, filteringEnabled) = [selectedPageSignal map:^id(id <WMLStorePage> storePage) {
        if ([storePage isKindOfClass:[WMLStoreHomePage class]]) {
            return @(YES);
        }

        if ([storePage isKindOfClass:[WMLStoreCategoryPage class]]) {
            switch (((WMLStoreCategoryPage *)storePage).category.categoryType) {
                case StoreCategoryTypeProduct:
                case StoreCategoryTypeSearch:
                case StoreCategoryTypeAll:
                case StoreCategoryTypeNewArrivals:
                case StoreCategoryTypeSale:
                    return @(YES);
                case StoreCategoryTypeSavedList:
                case StoreCategoryTypeCollection:
                default:
                    return @(NO);
            }
        }

        return @(NO);
    }];

    [self rac_liftSelector:@selector(_presentWonderpointsOnTopBarIfNeeded:) withSignals: [RACSignal combineLatest:@[selectedPageSignal, RACObserve(self, topBarMenuEnabled), RACObserve(self, categoryTitleWasScrolledAway)]],  nil];
}

- (void)_openContextDidChange:(WMLStoreOpenContext *)openContext {
    [self _loadStore:openContext.store];
}

- (void)_loadStore:(WMLStore *)store {
    self.loadingStore = YES;
    self.deals = store.deals;

    @weakify(self);
    [self.mallApiClient storeCategoryTreeWithStoreId:store.storeID success:^(WMLStoreCategoryTreeResponse *response) {
        @strongify(self);
        self.departments = response.departments;
        for (WMLStoreDepartment *department in self.departments) {
            department.store = self.openContext.store;
        }

        [(RACSubject *)self.storeDidLoad sendNext:nil];

        [self _selectInitialPage];

        self.loadingStore = NO;
    } failure:^(NSError *error) {
        @strongify(self);
        self.loadingStore = NO;

        [(RACSubject *)self.storeDidFailLoading sendNext:error];
    }];
}

- (void)_selectInitialPage {
    self.selectedPage = [self _initialPage];
}

- (id <WMLStorePage>)_initialPage {
    WMLStoreCategory *initialCategory = [self _initialCategory];
    if (initialCategory) {
        return [[WMLStoreCategoryPage alloc] initWithCategory:initialCategory];
    }

    if ([WMLFeatureFlagsStore sharedInstance].isStoreHomePageEnabled) {
        return [[WMLStoreHomePage alloc] init];
    }

    return [[WMLStoreCategoryPage alloc] initWithCategory:[self initialCategoryForDepartment:[self.departments firstObject]]];
}

- (WMLStoreCategory *)_initialCategory {
    WMLStoreCategory *openCategory = [self _categoryForStoreOpenCategory:self.openContext.categoryId];
    if (openCategory) {
        return openCategory;
    }

    WMLStoreCategory *lastVisitedCategory = [self.userProfile lastStoreCategoryPathForStore:self.openContext.store].category;
    return lastVisitedCategory ? lastVisitedCategory : [self _defaultCategory];
}

- (WMLStoreCategory *)_categoryForStoreOpenCategory:(NSString *)categoryId {
    if (!categoryId) {
        return nil;
    }
    WMLStoreDepartment *department = [self _departmentForRecentGender];
    if ([categoryId isEqualToString:WMLStoreOpenCategorySaleString]) {
        return department.saleCategory;
    } else if ([categoryId isEqualToString:WMLStoreOpenCategoryAllString]) {
        return department.allCategory;
    } else if ([categoryId isEqualToString:WMLStoreOpenCategoryNewArrivalsString]) {
        return department.newarrivalsCategory;
    } else {
        return [self.openContext.store departmentForDepartmentId:categoryId].newarrivalsCategory;
    }
}

- (WMLStoreDepartment *)_departmentForRecentGender {
    WMLStoreMallCategory *storeMallCategory = [self.mallCategoriesService.selectedCategory storeMallCategoryForStoreId:self.openContext.store.storeID];
    NSString *defaultDepartmentId, *defaultCategoryId;

    switch ([self _lastStoreDepartmentGender]) {
        case WMLStoreDepartmentGenderMen:
            defaultDepartmentId = storeMallCategory.menDepartmentID;
            defaultCategoryId = storeMallCategory.menCategoryID;
            break;
        case WMLStoreDepartmentGenderWomen:
            defaultDepartmentId = storeMallCategory.womenDepartmentID;
            defaultCategoryId = storeMallCategory.womenCategoryID;
            break;
        case WMLStoreDepartmentGenderNone:
            return self.openContext.store.departments.firstObject;
    }
    return [self.openContext.store departmentForDepartmentId:defaultDepartmentId];
}

- (WMLStoreCategory *)_defaultCategory {
    WMLStoreMallCategory *storeMallCategory = [self.mallCategoriesService.selectedCategory storeMallCategoryForStoreId:self.openContext.store.storeID];
    NSString *defaultDepartmentId, *defaultCategoryId;

    switch ([self _lastStoreDepartmentGender]) {
        case WMLStoreDepartmentGenderMen:
            defaultDepartmentId = storeMallCategory.menDepartmentID;
            defaultCategoryId = storeMallCategory.menCategoryID;
            break;
        case WMLStoreDepartmentGenderWomen:
            defaultDepartmentId = storeMallCategory.womenDepartmentID;
            defaultCategoryId = storeMallCategory.womenCategoryID;
            break;
        case WMLStoreDepartmentGenderNone:
        default:
            return nil;
    }

    if ([defaultDepartmentId hasAnyText]) {
        WMLStoreDepartment *department = [self.openContext.store departmentForDepartmentId:defaultDepartmentId];
        if (!department) {
            return nil;
        }

        return [self initialCategoryForDepartment:department];
    } else if ([defaultCategoryId hasAnyText]) {
        return [self.openContext.store categoryForCategoryId:defaultCategoryId];
    }

    return nil;
}

- (WMLStoreDepartmentGender)_lastStoreDepartmentGender {
    if ([UserObserverService sharedService].user.isGuest && [UserObserverService sharedService].user.profile.gender == WMLStoreDepartmentGenderNone) {
        return WMLStoreDepartmentGenderWomen;
    }

    return WMLStoreDepartmentGenderFromWMLUserGender([UserObserverService sharedService].user.profile.gender);
}

- (void)_presentWonderpointsOnTopBarIfNeeded:(id)state {
    if (!self.topBarMenuEnabled) {
        self.shouldPresentWonderpointsOnTopBar = NO;
        return;
    }

    if ([self.selectedPage isKindOfClass:[WMLStoreCategoryPage class]]) {
        switch (((WMLStoreCategoryPage *)self.selectedPage).category.categoryType) {
            case StoreCategoryTypeNewArrivals:
            case StoreCategoryTypeSale:
            case StoreCategoryTypeAll:
                self.shouldPresentWonderpointsOnTopBar = self.categoryTitleWasScrolledAway;
                break;
            case StoreCategoryTypeSearch:
            case StoreCategoryTypeProduct:
            default:
                self.shouldPresentWonderpointsOnTopBar = YES;
                break;
        }
    } else if ([self.selectedPage isKindOfClass:[WMLStoreHomePage class]]) {
        self.shouldPresentWonderpointsOnTopBar = YES;
    }
}

@end

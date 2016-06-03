//
// Created by lior on 26/2/13.
//
// To change the template use AppCode | Preferences | File Templates.
//

#import "WMLStoreViewController.h"

#import "MPDApiErrorView.h"
#import "NSString+Additions.h"
#import "Notification+WMLAdditions.h"
#import "StoreDealsTeaserViewController.h"
#import "StoreLoadingView.h"
#import "UIColor+HexString.h"
#import "UIColor+WMLAdditions.h"
#import "UIImage+WMLAdditions.h"
#import "UIView+Additions.h"
#import "UIView+WMLToast.h"
#import "WMLAccessibilityConstants.h"
#import "WMLAnalyticsService.h"
#import "WMLCart.h"
#import "WMLCartItem.h"
#import "WMLCartManagerController.h"
#import "WMLCartViewController.h"
#import "WMLCartViewControllerDelegate.h"
#import "WMLCategoryPreferences.h"
#import "WMLDeal.h"
#import "WMLDealOpenContext.h"
#import "WMLDealPopupManager.h"
#import "WMLDealSelectedCommand.h"
#import "WMLDismissNavigationNodeCommand.h"
#import "WMLFeatureFlagsStore.h"
#import "WMLFiltersPreferences.h"
#import "WMLFollowedStoresService.h"
#import "WMLInfoPopoverController.h"
#import "WMLIntroPopoverService.h"
#import "WMLMacros.h"
#import "WMLMall.h"
#import "WMLMallCart.h"
#import "WMLMallCategoriesService.h"
#import "WMLNavigationService.h"
#import "WMLNewsletterSubscriptionService.h"
#import "WMLNotificationPromptManager.h"
#import "WMLNotificationPromptRequest.h"
#import "WMLPresentGuestExperinceCommand.h"
#import "WMLProductOpenContext.h"
#import "WMLProductPath.h"
#import "WMLRouteBuilder.h"
#import "WMLSavedListCategory.h"
#import "WMLSavedProduct.h"
#import "WMLSavedProductsService.h"
#import "WMLSearchFacetsViewController.h"
#import "WMLShowProductCommand.h"
#import "WMLStoreBranding.h"
#import "WMLStoreCategoryPriceFilter.h"
#import "WMLStoreCategoryViewController.h"
#import "WMLStoreCategoryViewModel.h"
#import "WMLStoreCollectionCategory.h"
#import "WMLStoreCollectionsCategoryViewController.h"
#import "WMLStoreCollectionsCategoryViewModel.h"
#import "WMLStoreDealsViewController.h"
#import "WMLGenderCategory.h"
#import "WMLStoreFilterablePageViewModel.h"
#import "WMLStoreInfoResponse.h"
#import "WMLStoreMallCategory.h"
#import "WMLStoreOpenContext.h"
#import "WMLStoreProductSearchCategory.h"
#import "WMLStoreProductsCategoryViewController.h"
#import "WMLStoreProductsCategoryViewModel.h"
#import "WMLStoreProfile.h"
#import "WMLStoreViewModel.h"
#import "WMLUserHistory.h"
#import "WMLUserProfile.h"
#import "WMLVisitedStoresService.h"
#import "WMLWebViewOpenContext.h"
#import "WMLWonderModule.h"
#import "Wondermall-Swift.h"
#import <AFNetworking/UIImageView+AFNetworking.h>
#import <Vespucci/Vespucci.h>
#import <NSArray+BlocksKit.h>
#import "WMLStoreSearchCategoryViewController.h"
#import "WMLStorePageViewController.h"
#import "WMLStorePage.h"
#import "WMLStoreCategoryPage.h"
#import "WMLStoreHomePage.h"

@class WMLDealPopupViewController;

static const CGFloat kStoreTopFilterBarViewHeight = 44.0;

static const CGFloat kTopFilterBarCategorySelectButtonLabelExtraSpace = 25.0;

static const int kFiltersViewBottomMargin = 30;

static NSString *const kStoreAnalyticsEventSource = @"store";
static NSString *const kProductAnalyticsEventSource = @"product";

@interface WMLStoreViewController () <
        WMLCartViewControllerDelegate,
        UITabBarDelegate,
        StoreMenuManagerDelegate,
        HostViewController,
        StoreAdaptivityManagerDelegate,
        StatusBarManager>

@property (nonatomic, strong) WMLUserProfile *userProfile;
@property (nonatomic, strong) WMLUserHistory *userHistory;
@property (nonatomic, strong) WMLStoreOpenContext *openContext;
@property (nonatomic, strong) WMLStoreViewModel *viewModel;

@property (nonatomic, strong) UIView *containerView;
@property (nonatomic, strong) UIViewController <WMLStorePageViewController> *storePageViewController;
@property (nonatomic, strong) WMLSearchFacetsViewController *facetsViewController;
@property (nonatomic, weak) WMLCouponsViewController *couponsViewController;
@property (nonatomic, strong) WMLDealPopupManager *dealPopupManager;

@property (nonatomic, strong) TopToolbar *topFilterBar;

@property (nonatomic, strong) UIImageView *topBarImageView;
@property (nonatomic, strong) UIImageView *logoButtonImageView;

@property (nonatomic, strong) WMLBarButtonItem *savedListButton;

@property (nonatomic, strong) WMLStoreDealsViewController *storeDealsViewController;
@property (nonatomic, strong) StoreDealsTeaserViewController *storeDealsTeaserViewController;
@property (nonatomic, strong) MPDApiErrorView *errorView;

@property (nonatomic, strong) WMLStoreProfile *storeProfile;
@property (nonatomic, strong) StoreLoadingView *loadingView;
@property (nonatomic) BOOL isTopBarActive;
@property (nonatomic) UITabBar *tabBar;

@property (nonatomic, strong) WMLCartManagerController *cartViewController;
@property (nonatomic, strong) NSTimer *menuTimer;
@property (nonatomic, strong) RACDisposable *categoryLoadingErrorSignal;

@property (nonatomic, strong) WMLSavedProductsService *savedProductsService;
@property (nonatomic, weak) WMLInfoPopoverController *introPopover;

@property (nonatomic) StoreMenuManager *storeMenuManager;
@property (nonatomic) SidebarManager *sidebarManager;
@property (nonatomic) StoreAdaptivityManager *adaptivityManager;

@property (nonatomic) void (^searchTapHandler)();

@property (nonatomic) CollapsingManager *collapsingManager;

@end


@implementation WMLStoreViewController
@synthesize navigationNode = _navigationNode;
@synthesize statusBarHidden = _statusBarHidden;

#pragma mark - Init, Getters and Setters

objection_requires(
SQTypedKeyPath(WMLStoreViewController, userProfile),
SQTypedKeyPath(WMLStoreViewController, userHistory),
SQTypedKeyPath(WMLStoreViewController, savedProductsService))

#pragma mark - CollapsingManager

- (void)_disableCollapsing {
    self.collapsingManager.enabled = NO;
}

- (void)_enableCollapsingIfNeeded {
    [self.collapsingManager updateFrames];
    [self.collapsingManager enableIfNeeded];
}

- (void)_setupCollapsingBehaviour {
    if (self.collapsingManager != nil) {
        return;
    }
    if (![WMLFeatureFlagsStore sharedInstance].wondermall3Enabled) {
        return;
    }

    if (!self.navigationController) {
        return;
    }

    self.collapsingManager = [[CollapsingManager alloc] initWithViewController:self
                                                                      hostView:self.navigationController.view
                                                                    targetView:self.view
                                                                 navigationBar:self.navigationController.navigationBar
                                                                    scrollView:nil
                                                              statusBarManager:self
                                                                 viewsToManage:@[self.containerView]];
    self.collapsingManager.traitCollection = self.traitCollection;
}

#pragma mark - StatusBarManager

- (void)setStatusBarHidden:(BOOL)statusBarHidden {
    _statusBarHidden = statusBarHidden;
    [self setNeedsStatusBarAppearanceUpdate];
}

- (BOOL)prefersStatusBarHidden {
    return self.statusBarHidden;
}

- (UIStatusBarAnimation)preferredStatusBarUpdateAnimation {
    return UIStatusBarAnimationSlide;
}

#pragma mark - Init

objection_initializer(init)
- (instancetype)init {
    self = [super init];

    if (!self) {
        return nil;
    }

    [self _initialize];

    return self;
}

- (void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection {
    [super traitCollectionDidChange:previousTraitCollection];
    [self _customizationBasedOnSizeClasses];
}

- (void)_customizationBasedOnSizeClasses {
    if (self.traitCollection.horizontalSizeClass == UIUserInterfaceSizeClassCompact) {
        self.tabBar.hidden = NO;
        CGRect rect = self.view.bounds;
        rect.origin.y = 64;
        rect.size.height -= CGRectGetHeight(self.tabBar.frame) - 64;
        self.containerView.frame = rect;
    } else {
        self.tabBar.hidden = YES;
        CGRect frame = self.view.bounds;
        frame.origin.y = 64;
        frame.size.height -= 64;
        self.containerView.frame = frame;
    }

    [self _setupNavigationBarItems];
}

#pragma mark - UIViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    self.extendedLayoutIncludesOpaqueBars = YES;
    [[WMLDealService sharedInstance] fetchDeals];

    [self.viewModel loadStateFromNavigationNode:self.navigationNode];
    @weakify(self);
    [[RACObserve(self, navigationNode.parameters) distinctUntilChanged] subscribeNext:^(NSDictionary *parameters) {
        @strongify(self);
        NSString *newCategoryId = parameters[@"category"];
        if (newCategoryId) {
            [self _setPageForCategoryId:newCategoryId];
        }
    }];

    [self _setupAdaptivity];

    self.preferredStatusBarStyle = [self.store.branding.menuTopColor wml_isDarkColor] ? UIStatusBarStyleLightContent : UIStatusBarStyleDefault;

    // This view is used to cover the store collection view until its data are fully loaded.
    // BackgroundImageView cannot be used because it has a fade in animation which would reveal below subviews.
    CGRect frame = self.view.bounds;
    frame.origin.y += 64;
    frame.size.height -= 64;
    self.containerView = [[UIView alloc] initWithFrame:frame];
    self.containerView.autoresizingMask = (UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight);
    self.containerView.backgroundColor = self.store.branding.storeBackgroundColor;
    [self.view addSubview:self.containerView];

    self.view.backgroundColor = self.store.branding.storeBackgroundColor;

    self.loadingView = [[StoreLoadingView alloc] initWithFrame:self.view.bounds];
    [self.view addSubview:self.loadingView];

    [self _buildTopBars];
    [self _setupTabBar];
    [self _setupNavigationBarBranding];

    self.storeProfile = [self.userProfile storeProfileForStoreId:self.store.storeID];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(_storeFavoriteDidChangeNotificationName:)
                                                 name:WMLStoreFavoriteDidChangeNotificationName
                                               object:nil];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(_willResignActive:)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(_willEnterForeground:)
                                                 name:UIApplicationWillEnterForegroundNotification
                                               object:nil];

    [self.contentContainerView removeFromSuperview];
    [self.view addSubview:self.contentContainerView];
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];

    if (self.isBeingPresented || self.isMovingToParentViewController) {
        [self.loadingView displayAnimated:YES];
    }
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    if ([self isBeingPresented] || [self isMovingToParentViewController]) {
        [[WMLAnalyticsService defaultAnalytics] storeWillOpen:self.store];
    }
    [self _setupCollapsingBehaviour];
    [self _enableCollapsingIfNeeded];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    [self _disableCollapsing];
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [[CacheService sharedInstance] cancelNetworkingInBucket:[CacheService feedsBucket]];
    [self.categoryLoadingErrorSignal dispose];
    // free all the loaded departments data and loved products ids
    self.store.departments = nil;

    if(self.introPopover != nil) {
        [self.introPopover dismissPopoverAnimated:YES];
        self.introPopover = nil;
    }
}

#pragma mark - Public API

- (WMLBarButtonItem *)savedProductsButton {
    return self.savedListButton;
}

#pragma mark - Private

- (WMLCouponsViewController *)_couponsViewController {
    if (self.couponsViewController) {
        return self.couponsViewController;
    }
    WMLCouponsViewController *storeDealsViewController = [WMLCouponsViewController couponViewController];
    return storeDealsViewController;
}

- (void)_initialize {
    self.menuTimer = nil;
    self.dealPopupManager = [[JSObjection defaultInjector] getObjectWithArgs:[WMLDealPopupManager class], self, nil];
    self.viewModel = [[JSObjection defaultInjector] getObject:[WMLStoreViewModel class]];

    [self _bindViewModel];

    @weakify(self);
    [[[WMLUserHistory sharedInstance].leaveViewControllerSignal filter:^BOOL(id tuple) {
        @strongify(self);
        RACTupleUnpack(__unused UIViewController *leftViewController, UIViewController *destinateViewController) = tuple;
        return destinateViewController == self;
    }] subscribeNext:^(id _) {
        @strongify(self);
        [self _checkAndShowOpenProductWebViewIntroPopover];
    }];
}

- (void)_setupNavigationBarBranding {
    self.navigationController.navigationBar.tintColor = self.store.branding.menuTextColor;
    self.navigationController.navigationBar.barTintColor = self.store.branding.menuTopColor;
}

- (void)_setupTabBar {
    UIColor *textColor = self.store.branding.menuTextColor;

    self.tabBar = [[UITabBar alloc] init];
    self.tabBar.tintColor = textColor;
    self.tabBar.barTintColor = self.store.branding.menuTopColor;
    self.tabBar.translucent = NO;
    self.tabBar.delegate = self;

    self.tabBar.items = @[
        [self _feedsTabBarItemWithColor:textColor],
        [self _couponTabBarItemWithColor:textColor],
        [self _savedListTabBarItemWithColor:textColor]
    ];
    [self.view addSubview:self.tabBar];
    [self.tabBar mas_makeConstraints:^(MASConstraintMaker *make) {
        make.bottom.equalTo(self.view);
        make.left.equalTo(self.view);
        make.right.equalTo(self.view);
    }];
}

- (UIImage *)_deselectedTabBarImage:(UIImage *)image {
    return [[image wml_imageWithAlpha:0.7] imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
}

- (void)_setTabBarItemStyle:(UITabBarItem *)item withColor:(UIColor *)textColor {
    [item setTitleTextAttributes:@{
        NSForegroundColorAttributeName:textColor,
        NSFontAttributeName: [UIFont wml_fontOfSize:10]
    } forState:UIControlStateDisabled];
    [item setTitleTextAttributes:@{
        NSForegroundColorAttributeName:textColor,
        NSFontAttributeName: [UIFont wml_fontOfSize:10]
    } forState:UIControlStateSelected];
    [item setTitleTextAttributes:@{
        NSForegroundColorAttributeName:[textColor colorWithAlphaComponent:0.5],
        NSFontAttributeName: [UIFont wml_fontOfSize:10]
    } forState:UIControlStateNormal];
}

- (UITabBarItem *)_savedListTabBarItemWithColor:(UIColor *)textColor {
    UIImage *image = [[UIImage wml_imageNamed:@"store-tab-icon-saved" withColor:textColor] imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
    UITabBarItem *item = [[UITabBarItem alloc] initWithTitle:@"SAVED"
                                                       image:[self _deselectedTabBarImage:image]
                                                         tag:2];
    item.selectedImage = image;
    item.titlePositionAdjustment = UIOffsetMake(0.0, -2.0);
    [self _setTabBarItemStyle:item withColor:textColor];
    return item;
}

- (UITabBarItem *)_feedsTabBarItemWithColor:(UIColor *)textColor {
    UIImage *image = [[UIImage wml_imageNamed:@"store-tab-icon-products" withColor:textColor] imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
    UITabBarItem *item = [[UITabBarItem alloc] initWithTitle:@"PRODUCTS"
                                                       image:[self _deselectedTabBarImage:image]
                                                         tag:0];
    item.selectedImage = image;
    item.titlePositionAdjustment = UIOffsetMake(0.0, -2.0);
    [self _setTabBarItemStyle:item withColor:textColor];
    return item;
}

- (UITabBarItem *)_couponTabBarItemWithColor:(UIColor *)textColor {
    UIImage *image = [[UIImage wml_imageNamed:@"store-tab-icon-coupons" withColor:textColor] imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
    UITabBarItem *item = [[UITabBarItem alloc] initWithTitle:@"COUPONS" image:[self _deselectedTabBarImage:image] tag:1];
    item.selectedImage = image;
    item.titlePositionAdjustment = UIOffsetMake(0.0, -2.0);
    RACSignal *dealsCountSignal = [[RACObserve(self, viewModel.deals) map:^NSNumber*(NSArray *deals) {
        return @(deals.count);
    }] distinctUntilChanged];
    RAC(item, badgeValue) = [[dealsCountSignal map:^NSString*(NSNumber *dealsCount) {
        return dealsCount.integerValue > 0 ? dealsCount.stringValue : nil;
    }] deliverOnMainThread];
    RAC(item, enabled) = [[dealsCountSignal map:^NSValue*(NSNumber *dealsCount) {
        return @(dealsCount.integerValue > 0);
    }] deliverOnMainThread];
    [self _setTabBarItemStyle:item withColor:textColor];
    return item;
}

- (void)_setupAdaptivity {
    self.adaptivityManager = [[StoreAdaptivityManager alloc] initWithHostViewController:self navigationController:self.navigationController];
    self.adaptivityManager.delegate = self;
    self.adaptivityManager.traitCollection = self.traitCollection;
}

- (void)_bindViewModel {
    RAC(self, store) = RACObserve(self.viewModel, openContext.store);
    RAC(self, openContext) = RACObserve(self.viewModel, openContext);

    [self rac_liftSelector:@selector(_departmentsDidChange:) withSignals:[[RACObserve(self.viewModel, departments) skip:1] distinctUntilChanged], nil];
    [self rac_liftSelector:@selector(_dealsDidChange:) withSignals:[[RACObserve(self.viewModel, deals) skip:1] distinctUntilChanged], nil];
    [self rac_liftSelector:@selector(_storeDidLoad:) withSignals:self.viewModel.storeDidLoad, nil];
    [self rac_liftSelector:@selector(_storeDidFailLoading:) withSignals:self.viewModel.storeDidFailLoading, nil];
    [self rac_liftSelector:@selector(_selectedPageDidChange:) withSignals:[[RACObserve(self.viewModel, selectedPage) skip:1] distinctUntilChanged], nil];
}

- (void)_departmentsDidChange:(NSArray *)departments {
    self.store.departments = departments;
}

- (void)_dealsDidChange:(NSArray *)deals {
    self.store.deals = deals;

    [[NSNotificationCenter defaultCenter] postNotificationName:WMLDealsDidUpdateNotification
                                                        object:nil
                                                      userInfo:nil];
}

- (void)_storeDidLoad:(id)value {
    [self _buildStoreMenu];
    self.sidebarManager = [[SidebarManager alloc] init];
    if (self.openContext.deal) {
        [[[JSObjection defaultInjector] getObjectWithArgs:[WMLDealSelectedCommand class], self.openContext.deal, self.view, nil] execute];
    }

    [[WMLAnalyticsService defaultAnalytics] storeDidEnter:self.store
                                              eventSource:NSStringFromWMLOpenStoreEventSource(self.openContext.eventSource)];
}

- (void)_buildStoreMenu {
    self.storeMenuManager = [[StoreMenuManager alloc] initWithStore:self.store];
    self.storeMenuManager.delegate = self;
}

- (void)_storeDidFailLoading:(NSError *)error {
    [self.loadingView removeAnimated:NO];

    @weakify(self);
    [self displayStoreLoadErrorWithRetry:^{
        @strongify(self);
        [self.viewModel reloadStoreInfo];
    }];

    [[WMLAnalyticsService defaultAnalytics] loadingErrorDidOccurWithEventSource:@"load store"
                                                                          store:self.store];
}

- (void)_selectedPageDidChange:(id <WMLStorePage>)selectedPage {
    [self _updateLastVisitedStoreCategory];

    [self.loadingView displayAnimated:YES];

    [self _invalidatePageViewController];
}

- (void)_pageDidLoad:(id)value {
    [self.containerView insertSubview:self.storePageViewController.view belowSubview:self.topFilterBar];
    [self.loadingView removeAnimated:YES];
    if (!self.isTopBarActive) {
        [self _displayCollectionView];
    } else {
        if ([[WMLAnalyticsService defaultAnalytics] isTrackingEvent:kAnalyticsEventStoreCategoryClicked]) {
            [[WMLAnalyticsService defaultAnalytics] storeDidClickCategory:[self _viewModelStoreCategory] store:self.store];
        } else if ([[WMLAnalyticsService defaultAnalytics] isTrackingEvent:kAnalyticsEventStoreSearchSubmitted]) {
            // TODO: This is to be deleted when we migrate to the unified search screen.
            NSUInteger productsCount = 0;
            if ([self.storePageViewController.viewModel isKindOfClass:[WMLStoreSearchCategoryViewModel class]]) {
                productsCount = ((WMLStoreSearchCategoryViewModel *)self.storePageViewController.viewModel).exactResults.count;
            }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
            [[WMLAnalyticsService defaultAnalytics] storeDidSubmitSearchWithResultsCount:productsCount];
#pragma clang diagnostic pop
        }
    }
}

- (void)_invalidatePageViewController {
    UIViewController <WMLStorePageViewController> *storePageViewController = [self _pageViewControllerForStorePage:self.viewModel.selectedPage];
    id <WMLStorePageViewModel> storePageViewModel = [self _pageViewModelForStorePage:self.viewModel.selectedPage];
    storePageViewController.viewModel = storePageViewModel;
    [self _setStorePageViewControllerFrame:storePageViewController];

    @weakify(self);
    [storePageViewController.pageDidLoad subscribeNext:^(id x) {
        @strongify(self)
        [self.storePageViewController.view removeFromSuperview];
        self.storePageViewController = storePageViewController;
        [self _setStorePageViewControllerFrame:self.storePageViewController];

        [self _pageDidLoad:x];

        [self _setupNavigationBarItems];
        [self _buildAndDisplayOnlyInteractableStoreToolbarItems];
    }];
}

- (void)_setStorePageViewControllerFrame:(UIViewController *)storePageViewController {
    CGFloat tabBarHeight = self.tabBar.hidden ? 0 : CGRectGetHeight(self.tabBar.frame);
    storePageViewController.view.frame = CGRectMake(0, kStoreTopFilterBarViewHeight, self.view.width, self.view.height - kStoreTopFilterBarViewHeight - tabBarHeight);
}

- (UIViewController<WMLStoreCategoryViewController> *)_categoryPageViewControllerForCategory:(WMLStoreCategory *)category {
    switch (category.categoryType) {
        case StoreCategoryTypeProduct:
        case StoreCategoryTypeNewArrivals:
        case StoreCategoryTypeSale:
        case StoreCategoryTypeAll:
            return [[JSObjection defaultInjector] getObject:[WMLStoreProductsCategoryViewController class]];

        case StoreCategoryTypeSearch:
            return [[JSObjection defaultInjector] getObject:[WMLStoreSearchCategoryViewController class]];

        case StoreCategoryTypeCollection:
            return [[JSObjection defaultInjector] getObject:[WMLStoreCollectionsCategoryViewController class]];

        default:
            ALog(@"Unsupported category type %@", @(category.categoryType));
            return nil;
    }
}

- (UIViewController <WMLStorePageViewController> *)_pageViewControllerForStorePage:(id <WMLStorePage>)storePage {
    switch (storePage.pageType) {
        case WMLStorePageTypeHomepage:
            return [self _homePageViewController];
        case WMLStorePageTypeCategory: {
            UIViewController<WMLStoreCategoryViewController> *categoryViewController = [self _categoryPageViewControllerForCategory:((WMLStoreCategoryPage *)storePage).category];

            @weakify(self);
            [self rac_liftSelector:@selector(_openSubCategoryFromShortcut:) withSignals:categoryViewController.shortcutPressedSignal, nil];
            [categoryViewController.shortcutMoreButtonPressedSignal subscribeNext:^(__unused id x) {
                @strongify(self);
                [self _openStoreMenu];
            }];

            return categoryViewController;
        }
    }
}

- (StoreHomePageViewController *)_homePageViewController {
    StoreHomePageViewController *storeHomePageViewController = [StoreHomePageViewController storeHomePageViewController];
    [storeHomePageViewController.shortcutPressedSignal subscribeNext:^(id item) {
        [[WMLAnalyticsService defaultAnalytics] storeWillClickCategoryWithEventSource:kAnalyticsEventSourceStoreShortCutsView category:item];
        [self _openSubCategoryFromShortcut:item];
    }];
    [storeHomePageViewController.shortcutMoreButtonPressedSignal subscribeNext:^(id x) {
        [self _openStoreMenu];
    }];

    return storeHomePageViewController;
}

- (void)_openStoreMenu {
    WMLStoreCategory *category = (self.viewModel.selectedPage.pageType == WMLStorePageTypeHomepage) ? nil : [self _viewModelStoreCategory];
    [self.storeMenuManager presentMenuWithPresentingController:self selectedCategory:category];
}

- (void)_setupFiltering {
    self.facetsViewController = [WMLSearchFacetsViewController searchFacetsViewController];

    CGSize size = CGSizeMake(190, CGRectGetHeight(self.facetsViewController.view.frame));
    CGFloat maxFiltersViewHeight = self.view.height - kFiltersViewBottomMargin;
    if (size.height > maxFiltersViewHeight) {
        size.height = maxFiltersViewHeight;
    }

    self.facetsViewController.preferredContentSize = size;
    self.facetsViewController.view.frame = CGRectMake(0, 0, size.width, size.height);

    id<WMLStoreFilterablePageViewModel> filterableViewModel = (id<WMLStoreFilterablePageViewModel>)self.storePageViewController.viewModel;

    self.facetsViewController.sortOptions = filterableViewModel.sortOptions;

    RAC(self.facetsViewController, sort) = [[RACObserve(filterableViewModel, sort) distinctUntilChanged] map:^id(NSString *sort) {
        return @(WMLUniversalSearchSortTypeFromString(sort));
    }];

    RAC(filterableViewModel, sort) = [[[RACObserve(self.facetsViewController, sort) skip:1] distinctUntilChanged] map:^id(NSNumber *sortNumber) {
        return NSStringFromWMLUniversalSearchSortType(sortNumber.integerValue);
    }];

    RAC(filterableViewModel, facetSelections) = [[RACObserve(self.facetsViewController, facetSelections) skip:1] distinctUntilChanged];
    RAC(self.facetsViewController, facets) = [RACObserve(filterableViewModel, facets) distinctUntilChanged];

    @weakify(self);
    [[[RACObserve(filterableViewModel, filtering) skip:1] distinctUntilChanged] subscribeNext:^(NSNumber *filtering) {
        @strongify(self)
        if (filtering.boolValue) {
            [self.loadingView displayAnimated:YES];
        } else {
            [self.loadingView removeAnimated:YES];
        }
    }];
}

- (id <WMLStoreCategoryViewModel>)_categoryViewModelForCategory:(WMLStoreCategory *)category {
    switch (category.categoryType) {
        case StoreCategoryTypeProduct:
        case StoreCategoryTypeNewArrivals:
        case StoreCategoryTypeSale:
        case StoreCategoryTypeAll:
            return [[WMLStoreProductsCategoryViewModel alloc] initWithCategory:category];

        case StoreCategoryTypeSearch:
            return [[WMLStoreSearchCategoryViewModel alloc] initWithCategory:(WMLStoreProductSearchCategory *)category];

        case StoreCategoryTypeCollection:
            return [[WMLStoreCollectionsCategoryViewModel alloc] initWithCategory:category];

        default:
            ALog(@"Unsupported category type %@", @(category.categoryType));
            return nil;
    }
}

- (id <WMLStorePageViewModel>)_pageViewModelForStorePage:(id <WMLStorePage>)storePage {
    switch (storePage.pageType) {
        case WMLStorePageTypeHomepage:
            return [[StoreHomePageViewModel alloc] initWithStore:self.openContext.store];
        case WMLStorePageTypeCategory:
            return [self _categoryViewModelForCategory:((WMLStoreCategoryPage *)storePage).category];
    }
}

- (void)_presentDealPopupControllerWithDealOpenContext:(WMLDealOpenContext *)dealOpenContext {
    [self.dealPopupManager presentDealPopupForDealContext:dealOpenContext];
}

- (void)_buildTopBars {
    self.topFilterBar = [[TopToolbar alloc] initWithFrame:CGRectMake(0, 1, self.view.width, kStoreTopFilterBarViewHeight)];  // Start below the navigation bar's border line
    self.topFilterBar.barTintColor = [UIColor wml_warmWhite];
    self.topFilterBar.autoresizingMask = UIViewAutoresizingFlexibleWidth;

    self.topBarImageView = [[UIImageView alloc] init];

    if (self.store.branding.topBarBackgroundUrl) {
        NSURLRequest *imageRequest = [[NSURLRequest alloc] initWithURL:self.store.branding.topBarBackgroundUrl];
        @weakify(self);
        [self.topBarImageView setImageWithURLRequest:imageRequest
                                    placeholderImage:nil
                                             success:^(NSURLRequest *request, NSHTTPURLResponse *response, UIImage *image) {
                                                 @strongify(self);
                                                 self.topBarImageView.image = [image resizableImageWithCapInsets:UIEdgeInsetsZero resizingMode:UIImageResizingModeStretch];
                                                 [self.navigationController.navigationBar setBackgroundImage:self.topBarImageView.image forBarMetrics:UIBarMetricsDefault];
                                             }
                                             failure:nil];
    } else {
        [self.navigationController.navigationBar setBackgroundImage:[UIImage wml_imageFromColor:self.store.branding.menuTopColor] forBarMetrics:UIBarMetricsDefault];
    }

    [self _setupNavigationBarItems];

    [self.containerView addSubview:self.topFilterBar];

    [self _deactivateTopBar];
}

- (void)_reportRefineMenuIsOpen:(BOOL)isOpen {
    id<WMLStoreFilterablePageViewModel> filterableViewModel = (id<WMLStoreFilterablePageViewModel>)self.storePageViewController.viewModel;

    if (isOpen) {
        [[WMLAnalyticsService defaultAnalytics] storeDidClickRefine:self.store facetSelections:filterableViewModel.facetSelections numberOfItems:filterableViewModel.numberOfItems];
    } else {
        [[WMLAnalyticsService defaultAnalytics] storeDidCloseRefine:self.store category:[self _viewModelStoreCategory] facetSelections:filterableViewModel.facetSelections numberOfItems:filterableViewModel.numberOfItems];
    }
}

- (void)_reportExitStore:(NSString *)eventSource {
    if (self.viewModel.selectedPage.pageType == WMLStorePageTypeCategory) {
        [[WMLAnalyticsService defaultAnalytics] storeDidExitWithCategory:[self _viewModelStoreCategory]];
    } else {
        [[WMLAnalyticsService defaultAnalytics] storeDidExitWithEventSource:eventSource];
    }
}

- (void)_changeSaveStateForProductPath:(WMLProductPath *)productPath
                            fromSource:(NSString *)source {

    if ([WMLPresentGuestExperinceCommand promptGuestExperinceAuthenticationIfNeededWithSource:kAnalyticsEventSourceGuestViewSaveProduct]) {
        return;
    }

    WMLSavedProduct *savedProduct = [WMLSavedProduct savedProductForProduct:productPath.product];
    BOOL isProductSaved = [self.savedProductsService isProductSaved:productPath.product];
    if (isProductSaved) {
        @weakify(self);
        [self.savedProductsService removeSavedProducts:[NSSet setWithObject:savedProduct] success:nil failure:^{
           @strongify(self);
           [self.view wml_showFailToastWithMessage:NSLocalizedString(@"Sorry, could not remove this item. Try again", nil)];
        }];

        [[WMLAnalyticsService defaultAnalytics] savedProductDidUnsave:savedProduct
                                                             category:productPath.category
                                                      facetSelections:[self _activeFacetSelections]
                                                          eventSource:source];
    } else {
        [self.savedProductsService saveProducts:[NSSet setWithObject:savedProduct] success:^{
            WMLNotificationPromptRequest *notificationPromptRequest = [WMLNotificationPromptRequest request];
            notificationPromptRequest.notificationType = WMLNotificationForSaleOnSavedItems;
            notificationPromptRequest.eventSource = kAnalyticsEventSourceSaveProduct;

            [[WMLNotificationPromptManager sharedInstance] promptForNotificationWithRequest:notificationPromptRequest];
        } failure:^{
            [self.view wml_showFailToastWithMessage:NSLocalizedString(@"Sorry, could save this item. Try again", nil)];
        }];

        [[WMLAnalyticsService defaultAnalytics] productDidSave:savedProduct
                                                      category:productPath.category
                                                      facetSelections:[self _activeFacetSelections]
                                                   eventSource:source
                                                        screen:kAnalyticsParameterScreenGrid];
    }
}


- (NSArray *)_activeFacetSelections {
    if ([self.storePageViewController.viewModel conformsToProtocol:@protocol(WMLStoreFilterablePageViewModel)]) {
        return ((id<WMLStoreFilterablePageViewModel>)self.storePageViewController.viewModel).facetSelections;
    }

    return nil;
}

- (void)_displayCollectionView {
    @weakify(self);
    [UIView animateWithDuration:kWMLAnimationDurationSlow delay:0.0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
        @strongify(self);
        self.storePageViewController.view.alpha = 1.0;
        [self _activateTopBar];
    } completion:^(BOOL finished) {
        @strongify(self);
        [[WMLAnalyticsService defaultAnalytics] storeDidOpenWithCategory:[self _viewModelStoreCategory]];
    }];
}

- (void)_deactivateTopBar {
    self.viewModel.topBarMenuEnabled = NO;
    self.isTopBarActive = NO;
    self.topFilterBar.hidden = YES;
}

- (void)_activateTopBar {
    self.viewModel.topBarMenuEnabled = YES;
    self.topFilterBar.hidden = NO;
    self.isTopBarActive = YES;
}

- (void)_willResignActive:(NSNotification *)notification {
    [self _reportExitStore:@"resign active"];
}

- (void)_willEnterForeground:(NSNotification *)notification {
    [[WMLAnalyticsService defaultAnalytics] applicationDidReturnFromBackgroundToStore:self.store];
}

- (void)_checkAndShowOpenProductWebViewIntroPopover {
    if ([[WMLUserHistory sharedInstance] isEventMarked:kUserEventDidOpenProductInWebView]) {
        @weakify(self);
        [WMLIntroPopoverService checkFirstTimeAction:WMLUserActionReturnFromProductWebView showIntroPopover:^{
            @strongify(self);
            NSString *title = [NSString stringWithFormat:NSLocalizedString(@"The %@ cart is here", nil), self.store.domainName];
            NSString *description = NSLocalizedString(@"Whenever you're ready to checkout...", nil);
            [WMLInfoPopoverController showPopoverWithTitle:title
                                               description:description
                                                     width:450
                                                    atRect:self.navigationItem.rightBarButtonItems.firstObject.customView.frame // The cart is always the first button
                                               arrowOffset:205
                                                    inView:self.view
                                  permittedArrowDirections:UIPopoverArrowDirectionUp];
        }];
    }
}

- (void)_setLastCategoryGenderIfNeededWithCategory:(WMLStoreCategoryPath *)categoryPath {
    User *user = [UserObserverService sharedService].user;
    if (categoryPath.department.gender != WMLStoreDepartmentGenderNone && user.isGuest) {
        user.profile.gender = WMLUserGenderFromWMLStoreDepartmentGender(categoryPath.department.gender);
    }
}

- (WMLStoreCategory *)_viewModelStoreCategory {
    switch (self.viewModel.selectedPage.pageType) {
        case WMLStorePageTypeHomepage: {
            return [self.viewModel initialCategoryForDepartment:self.store.departments.firstObject];
        }
        case WMLStorePageTypeCategory: {
            return ((WMLStoreCategoryPage *)self.viewModel.selectedPage).category;
        }
    }
}

- (void)_openSubCategoryFromShortcut:(id)item {
    if ([item isKindOfClass:[WMLStoreDepartment class]]) {
        [self _setSelectedPageForDepartment:item];
    } else if ([item isKindOfClass:[WMLStoreCategory class]]) {
        [[WMLAnalyticsService defaultAnalytics] storeWillClickCategoryWithEventSource:kAnalyticsEventSourceStoreShortCutsView category:item];
        self.viewModel.selectedPage = [[WMLStoreCategoryPage alloc] initWithCategory:((WMLStoreCategory *)item)];
    }
}

- (void)_setSelectedPageForDepartment:(WMLStoreDepartment *)department {
    if ([department.name isEqualToString:kWMLStoreAllProductsDepartmentName]) {
        self.viewModel.selectedPage = [[WMLStoreHomePage alloc] init];
    } else {
        self.viewModel.selectedPage = [[WMLStoreCategoryPage alloc] initWithCategory:department.allCategory];
    }
}

- (void)_setPageForCategoryId:(NSString *)categoryId {
    // TODO: We shouldn't guess whether it's a department or a category. Waiting for categories refactoring
    WMLStoreDepartment *department = [self.store departmentForDepartmentId:categoryId];
    if (department) {
        [self _setSelectedPageForDepartment:department];
    } else {
        WMLStoreCategory *category = [self.store categoryForCategoryId:categoryId];
        NSAssert(category != nil, @"No category found for id %@", categoryId);
        if (category) {
            self.viewModel.selectedPage = [[WMLStoreCategoryPage alloc] initWithCategory:category];
        }
    }
}

#pragma mark - UI Builders

- (WMLBarButtonItem *)_closeStoreButton {
    @weakify(self);
    return [WMLBarButtonItem buttonWithCustomView:[[UIImageView alloc] initWithImage:[UIImage imageNamed:@"toolbar-icon-close"]] tapHandler:^{
        @strongify(self);
        [self _reportExitStore:@"close button"];
        [self.navigationNode dismiss];
    }];
}

- (WMLBarButtonItem *)_storeLogoButton {
    @weakify(self);
    CGFloat height = self.traitCollection.horizontalSizeClass == UIUserInterfaceSizeClassCompact ? 30 : 45;
    WMLBarButtonItem *barButtonItem;
    if (!self.logoButtonImageView) {
        self.logoButtonImageView = [[UIImageView alloc] init];
        self.logoButtonImageView.contentMode = UIViewContentModeScaleAspectFit;
        [self.logoButtonImageView wml_setImageWithURL:self.store.branding.logoWideUrl bucket:[CacheService brandingBucket] completion:^(UIImage * _Nullable image, SDImageCacheType _) {
            @strongify(self);
            self.logoButtonImageView.image = image;
            [self.logoButtonImageView sizeToFit];
            if (image == nil) {
                return;
            }
            WMLSetStructure(self.logoButtonImageView, frame, {
                CGFloat orignalRatio = image.size.height / image.size.width;
                frame.size.width = height / orignalRatio;
                frame.origin.x -= 30;
                CGRectIntegral(frame);
            });
            barButtonItem.width = CGRectGetWidth(self.logoButtonImageView.frame);
        }];
    }
    barButtonItem = [WMLBarButtonItem buttonWithCustomView:self.logoButtonImageView tapHandler:^{
        @strongify(self);
        [self.viewModel openHomepage];
    }];
    return barButtonItem;
}

- (WMLUniversalSearchBar *)_searchBar {
    WMLUniversalSearchBar *searchBar = [[WMLUniversalSearchBar alloc] initWithFrame:CGRectMake(0, 0, 260, 30)];
    searchBar.placeholder = [@"Search " stringByAppendingString:self.store.name];
    [searchBar.subviews enumerateObjectsUsingBlock:^(__kindof UIView * _Nonnull subview, NSUInteger idx, BOOL * _Nonnull stop) {
        subview.userInteractionEnabled = false;
    }];
    UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(_searchButtonAction)];
    [searchBar addGestureRecognizer:tapGesture];
    return searchBar;
}

- (void)_searchButtonAction {
    self.searchTapHandler();
}

- (WMLBarButtonItem *)_shoppingCartButtonWithColor:(UIColor *)color {
    BadgedCart *badgedCartView = [[BadgedCart alloc] initWithTapHandler:nil];
    WMLSetStructure(badgedCartView, frame, {
        frame.size.width -= 16;
    });
    badgedCartView.imageView.contentMode = UIViewContentModeCenter;
    badgedCartView.badgeColor = color;
    badgedCartView.cartColor = color;

    @weakify(self);
    [[WMLTabBarBadgeObserverService sharedInstance] observeStoreCartBadgeForStore:self.store observer:^(NSInteger badgeCount, BOOL enabled) {
        badgedCartView.alpha = enabled ? 1 : 0.2;
        badgedCartView.badge = badgeCount;
    }];

    return [WMLBarButtonItem buttonWithCustomView:badgedCartView tapHandler:^{
        @strongify(self);
        if ([WMLPresentGuestExperinceCommand promptGuestExperinceAuthenticationIfNeededWithSource:kAnalyticsEventSourceGuestViewAddToCart]) {
            return;
        }
        [[WMLAnalyticsService defaultAnalytics] storeDidClickCartWithStore:self.store
                                                               eventSource:kAnalyticsEventSourceStore];
        NSURL *URL = [[WMLRouteBuilder sharedRouteBuilder] cartURLForStoreId:self.store.storeID];
        [[WMLNavigationService sharedService] navigateToURL:URL];
    }];
}

- (WMLBarButtonItem *)_searchButton {
    UIImageView *imageView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"toolbar-icon-search"]];
    imageView.contentMode = UIViewContentModeCenter;
    WMLSetStructure(imageView, frame, {
        frame.size.width -= 8;
    });
    return [WMLBarButtonItem buttonWithCustomView:imageView tapHandler:self.searchTapHandler];
}

- (void)_setupSearchHandler {
    @weakify(self);
    self.searchTapHandler = ^{
        @strongify(self);
        [[WMLAnalyticsService defaultAnalytics] searchDidClickSearchWithStore:self.viewModel.openContext.store];

        NSURL *URL = [[WMLRouteBuilder sharedRouteBuilder] universalSearchURLWithQuery:nil storeId:self.store.storeID];
        [[WMLNavigationService sharedService] navigateToURL:URL];
    };
}

- (WMLBarButtonItem *)_wonderPointsButton {
    @weakify(self);
    return [WMLBarButtonItem buttonWithCustomView:[[UIImageView alloc] initWithImage:UIImageForCartFromStoreMultiplier(self.store.rewardsMultiplier)] tapHandler:^{
        @strongify(self);
        [[[AboutWonderPointsCommand alloc] initWithStore:self.store source:@"Store"] execute];
    }];
}

- (WMLBarButtonItem *)_dealsButton {
    BadgedImage *dealsBadgedView = [[BadgedImage alloc] init];
    dealsBadgedView.imageView.contentMode = UIViewContentModeCenter;
    WMLSetStructure(dealsBadgedView, frame, {
        frame.size.width -= 16;
    });
    dealsBadgedView.image = [UIImage imageNamed:@"toolbar-icon-coupons"];

    @weakify(self);
    WMLBarButtonItem *barButtonItem = [WMLBarButtonItem buttonWithCustomView:dealsBadgedView tapHandler:^{
        @strongify(self);
        [[WMLAnalyticsService defaultAnalytics] dealListDidOpenFromScreen:kStoreAnalyticsEventSource
                                                                    store:self.store
                                                            teaserVisible:NO];
        NSURL *panelURL = [[WMLRouteBuilder sharedRouteBuilder] couponsStorePopUpForStoreId:self.openContext.store.storeID];
        [[WMLNavigationService sharedService] navigateToURL:panelURL];
    }];

    RACSignal *dealsCountSignal = [[RACObserve(self, store.deals) map:^NSNumber*(NSArray *deals) {
        return @(deals.count);
    }] distinctUntilChanged];
    RAC(dealsBadgedView, badge) = [dealsCountSignal deliverOnMainThread];
    RAC(barButtonItem, enabled) = [[dealsCountSignal map:^NSValue*(NSNumber* dealsCount) {
        return @([dealsCount integerValue] > 0);
    }] deliverOnMainThread];
    return barButtonItem;
}

- (WMLBarButtonItem *)_savedListButton {
    @weakify(self);
    UIImageView *imageView = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"toolbar-icon-saved"]];
    imageView.contentMode = UIViewContentModeCenter;
    WMLSetStructure(imageView, frame, {
        frame.size.width -= 16;
    });
    return [WMLBarButtonItem buttonWithCustomView:imageView tapHandler:^{
        @strongify(self);
        [[WMLAnalyticsService defaultAnalytics] savedListDidOpenWithEventSource:kStoreAnalyticsEventSource
                                                                          store:self.store
                                                          numberOfSavedProducts:[self.savedProductsService numberOfSavedProductsInStore:self.store]
                                                    numberOfSavedProductsOnSale:[self.savedProductsService numberOfSavedProductsOnSaleInStore:self.store]];

        NSURL *URL = [[WMLRouteBuilder sharedRouteBuilder] savedListURLForStore:self.store.storeID];
        [[WMLNavigationService sharedService] navigateToURL:URL];
    }];
}

- (void)_setupNavigationBarItems {
    [self _resetNavigationBarItems];
    UINavigationItem *storeNavigationItem = self.navigationItem;
    NSMutableArray *leftBarButtonItems = [NSMutableArray array];
    [leftBarButtonItems addObject:[UIBarButtonItem buttonItemLeftEdgeMargin]];
    UIBarButtonItem *closeStore = [self _closeStoreButton];
    [leftBarButtonItems addObject:closeStore];
    UIBarButtonItem *storeLogo = [self _storeLogoButton];
    [leftBarButtonItems addObject:storeLogo];

    storeNavigationItem.hidesBackButton = YES;
    storeNavigationItem.leftBarButtonItems = leftBarButtonItems;

    if (self.store.departments == nil) {
        return;
    }

    BOOL isHorizontallyCompact = self.traitCollection.horizontalSizeClass == UIUserInterfaceSizeClassCompact;
    [self _setupSearchHandler];
    storeNavigationItem.titleView = isHorizontallyCompact ? nil : [self _searchBar];

    NSMutableArray<UIBarButtonItem *> *rightBarButtonItems = [NSMutableArray array];

    [rightBarButtonItems addObject:[UIBarButtonItem buttonItemWithFixedWidth:-8]];
    WMLBarButtonItem *shoppingCart = [self _shoppingCartButtonWithColor:self.store.branding.menuTextColor];
    [rightBarButtonItems addObject:shoppingCart];

    if (isHorizontallyCompact) {
        WMLBarButtonItem *search = [self _searchButton];
        [rightBarButtonItems addObject:search];

        if ([WonderpointsFeatureFlags sharedInstance].rewardsInStoreEnabled) {
            WMLBarButtonItem *wonderpoints = [self _wonderPointsButton];
            [rightBarButtonItems addObject:wonderpoints];
        }
    } else {
        WMLBarButtonItem *deals = [self _dealsButton];
        [rightBarButtonItems addObject:deals];
        WMLBarButtonItem *savedList = [self _savedListButton];
        [rightBarButtonItems addObject:savedList];
        if (self.viewModel.selectedPage.pageType != WMLStorePageTypeHomepage) {
            WMLBarButtonItem *wonderpoints = [self _wonderPointsButton];
            [rightBarButtonItems addObject:wonderpoints];
        }
    }

    storeNavigationItem.rightBarButtonItems = rightBarButtonItems;
}

- (void)_resetNavigationBarItems {
    UINavigationItem *storeNavigationItem = self.navigationItem;

    storeNavigationItem.leftBarButtonItems = @[];
    storeNavigationItem.titleView = nil;
    storeNavigationItem.rightBarButtonItems = @[];
}

- (UIBarButtonItem *)_buildCategoryBackButtonItem {
    CategoryBackButtonItem *item = [[CategoryBackButtonItem alloc] init];
    item.tintColor = [UIColor wml_mediumGray];
    WMLStoreCategory *category = ((WMLStoreCategoryPage *)self.viewModel.selectedPage).category;
    if (category.parent) {
        item.categoryName = category.parent.name;
    } else {
        item.categoryName = category.shouldNavigateToRootDepartment ? @"All Departments" : category.department.name;
    }
    item.toolbar = self.topFilterBar;
    @weakify(self);
    item.tapHandler = ^{
        @strongify(self);
        if (self.viewModel.selectedPage.pageType != WMLStorePageTypeCategory) {
            return;
        }

        WMLStoreCategory *category = ((WMLStoreCategoryPage *)self.viewModel.selectedPage).category;
        NSString *parentCategoryId;
        if (category.parent) {
            parentCategoryId = category.parent.storeCategoryID;
        } else {
            parentCategoryId = category.shouldNavigateToRootDepartment ? self.store.departments.firstObject.storeDepartmentID : category.department.storeDepartmentID;
        }
        [self _setPageForCategoryId:parentCategoryId];
        [self _buildStoreMenu];

    };
    return item;
}

- (CategorySelectButton *)_buildCategorySelectButtonView {
    CategorySelectButton *categorySelectButtonView = [CategorySelectButton categorySelectButton];

    if ([self.storePageViewController.viewModel isKindOfClass:[WMLStoreSearchCategoryViewModel class]]) {
        categorySelectButtonView.categoryLabel.text = [((WMLStoreSearchCategoryViewModel *)self.storePageViewController.viewModel).category.query capitalizedString];
    } else {
        categorySelectButtonView.categoryLabel.text = [self _viewModelStoreCategory].name ?: kWMLStoreAllProductsDepartmentName;
    }
    [categorySelectButtonView.categoryLabel sizeToFit];
    categorySelectButtonView.frame = ({
        CGRect frame = categorySelectButtonView.frame;
        frame.size.width = CGRectGetWidth(categorySelectButtonView.categoryLabel.frame) + kTopFilterBarCategorySelectButtonLabelExtraSpace;
        frame;
    });

    return categorySelectButtonView;
}

- (WMLBarButtonItem *)_buildCategorySelectButton {
    CategorySelectButton *categorySelectButtonView = [self _buildCategorySelectButtonView];

    @weakify(self);
    return [WMLBarButtonItem buttonWithCustomView:categorySelectButtonView tapHandler:^{
        @strongify(self);
        [self _openStoreMenu];
        [[WMLAnalyticsService defaultAnalytics] storeDidClickBrowseButton:self.store
                                                                 typeName:[self.viewModel pageTypeName]];
    }];
}

- (FilterBarButtonItem *)_buildFilterButton {
    FilterBarButtonItem *filterButton = [[FilterBarButtonItem alloc] init];
    filterButton.tintColor = [UIColor wml_mediumGray];
    filterButton.toolbar = self.topFilterBar;
    @weakify(self);
    filterButton.tapHandler = ^{
        @strongify(self);
        [self.sidebarManager presentSidebarWithPresentingController:self viewController:self.facetsViewController dismissHandler:^{
            [self _reportRefineMenuIsOpen:NO];
        }];
        [self _reportRefineMenuIsOpen:YES];
    };

    RAC(filterButton, filtersOnIconVisible) = [RACObserve(((id <WMLStoreFilterablePageViewModel>)self.storePageViewController.viewModel), facetSelections) map:^id(NSArray *facetSelections) {
        return @(facetSelections && facetSelections.count);
    }];

    return filterButton;
}

- (void)_buildAndDisplayOnlyInteractableStoreToolbarItems {
    self.topFilterBar.items = @[];

    WMLBarButtonItem *categorySelectButton = [self _buildCategorySelectButton];
    FilterBarButtonItem *filterButton = [self _buildFilterButton];
    UIBarButtonItem *flexibleSpaceItem = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFlexibleSpace target:nil action:nil];

    NSArray<UIBarButtonItem *> *categorySelectButtonItems = [[NSArray alloc] initWithObjects:flexibleSpaceItem, categorySelectButton, flexibleSpaceItem, nil];
    NSMutableArray<UIBarButtonItem *> *storeToolbarItems = [[NSMutableArray alloc] init];

    if (self.viewModel.selectedPage.pageType == WMLStorePageTypeCategory) {
        WMLStoreCategory *category = ((WMLStoreCategoryPage *)self.viewModel.selectedPage).category;
        if (!(category.isDepartmentCategory && category.department.store.isSingleDepartmentStore)) {
            [storeToolbarItems addObject:[UIBarButtonItem buttonItemLeftEdgeMargin]];
            UIBarButtonItem *categoryBackButton = [self _buildCategoryBackButtonItem];
            [storeToolbarItems addObject:categoryBackButton];
        }
    }

    [storeToolbarItems addObjectsFromArray:categorySelectButtonItems];

    if ([self.storePageViewController.viewModel conformsToProtocol:@protocol(WMLStoreFilterablePageViewModel)] && self.viewModel.isFilteringEnabled) {
        [self _setupFiltering];
        [storeToolbarItems addObject:filterButton];
        [storeToolbarItems addObject:[UIBarButtonItem buttonItemRightEdgeMargin]];
    }

    self.topFilterBar.items = storeToolbarItems;
}

#pragma mark - WMLCartViewControllerDelegate

- (void)cartViewController:(WMLCartViewController *)cartViewController
 didSelectViewCartForStore:(WMLStore *)store {
    if ([store isSisterStore:self.store]) {
        // no-op
        return;
    }
    [self.cartDelegate cartViewController:cartViewController
                didSelectViewCartForStore:store];
}

- (void)cartViewController:(WMLCartViewController *)cartViewController
            didSelectStore:(WMLStore *)store {
    if ([store isEqualToStore:self.store]) {
        // no-op
        return;
    }
    [self.cartDelegate cartViewController:cartViewController
                          didSelectStore:store];
}

- (void)cartViewController:(WMLCartViewController *)cartViewController
           didCheckoutCart:(WMLCart *)cart {
    if (![cart containsStoreFromMerchantGroup:self.store]) {
        [self.cartDelegate cartViewController:cartViewController
                              didCheckoutCart:cart];
        return;
    }
}

- (void)cartViewController:(WMLCartViewController *)cartViewController
             didSelectItem:(WMLCartItem *)cartItem {
    NSString *eventSource = [NSString stringWithFormat:@"%@-%@", kAnalyticsEventSourceStore, kAnalyticsEventSourceCart];
    WMLProductOpenContext *productOpenContext = [WMLProductOpenContext contextWithSearchProductVariant:cartItem.variant
                                                                                              category:[WMLStoreCategory cartCategory]
                                                                                           eventSource:eventSource
                                                                                            openSource:WMLProductOpenSourceStore
                                                                                        positionInView:1
                                                                                      totalItemsInView:1];

    [[[WMLShowProductCommand alloc] initWithProductContext:productOpenContext] execute];
}

#pragma mark - WMLDealPopupViewControllerDelegate

- (void)dealPopupViewController:(WMLDealPopupViewController *)dealPopupViewController
     didRequestActionForDeal:(WMLDeal *)deal {

    [self.dealPopupManager dismissDealPopup];

    [[[JSObjection defaultInjector] getObjectWithArgs:[WMLDealSelectedCommand class], deal, self.view, nil] execute];
}

#pragma mark - API Error handling

- (void)displayStoreLoadErrorWithRetry:(void (^)(void))retryBlock {
    if (self.errorView) {
        [self.errorView removeFromSuperview];
    }
    self.errorView = [MPDApiErrorView mpdStoreErrorView];
    self.errorView.frame = CGRectMake(CGRectGetMinX(self.containerView.frame),
                                      CGRectGetMaxY(self.topBarImageView.frame),
                                      CGRectGetWidth(self.containerView.frame),
                                      CGRectGetHeight(self.containerView.frame) - CGRectGetHeight(self.topBarImageView.frame));
    [self.errorView setStyleWithTextColor:self.store.branding.menuTopColor
                          backgroundColor:self.store.branding.storeBackgroundColor];

    @weakify(self);
    [self.errorView.tryAgainActionSignal subscribeNext:^(__unused id x) {
        @strongify(self);
        [self.errorView removeFromSuperview];
        if (retryBlock) {
            retryBlock();
        }
    }];

    [self.view insertSubview:self.errorView belowSubview:self.topBarImageView];
}

- (void)_updateLastVisitedStoreCategory {
    if (self.viewModel.selectedPage.pageType == WMLStorePageTypeCategory) {
        WMLStoreCategory *category = ((WMLStoreCategoryPage *)self.viewModel.selectedPage).category;
        WMLStoreCategoryPath *categoryPath = [WMLStoreCategoryPath storeCategoryPathWithStore:category.department.store categoryId:category.storeCategoryID];
        if (categoryPath) {
            [[WMLVisitedStoresService sharedInstance] updateVisitedStoreFromCategoryPath:categoryPath];
        }
    }
}

#pragma mark - StoreMenuManagerDelegate

- (void)storeMenuManager:(StoreMenuManager *)manager didSelectCategoryId:(NSString *)categoryId {
    [self _setPageForCategoryId:categoryId];
}

- (void)storeMenuManager:(StoreMenuManager *)manager didOpenURL:(NSURL *)URL {
    [[[PresentSafariCommand alloc] initWithUrl:URL] execute];
}

- (void)_categorySelected:(WMLStoreCategoryPath *)categoryPath {
    if ([categoryPath.category isEqual:[self _viewModelStoreCategory]]) {
        return;
    }

    [self _setLastCategoryGenderIfNeededWithCategory:categoryPath];

    [[WMLVisitedStoresService sharedInstance] updateVisitedStoreFromCategoryPath:categoryPath];

    // TODO: PT #99676870 - handle loading of persisted user filters.

    self.viewModel.selectedPage = [[WMLStoreCategoryPage alloc] initWithCategory:categoryPath.category];
}

#pragma mark - StoreAdaptivityManagerDelegate

- (void)storeAdaptivityManager:(StoreAdaptivityManager *)storeAdaptivityManager didSelectPage:(WMLStorePage)page {
    NSUInteger index = page;
    self.tabBar.selectedItem = self.tabBar.items[index];
}

#pragma mark - UITabBarDelegate

- (void)tabBar:(UITabBar *)tabBar didSelectItem:(UITabBarItem *)item {
    if (tabBar != self.tabBar) {
        return;
    }
    WMLStorePage page = item.tag;
    switch (page) {
        case WMLStorePageProducts:
            [self.navigationNode.child dismiss];
            break;
        case WMLStorePageCoupons: {
            NSURL *URL = [[WMLRouteBuilder sharedRouteBuilder] couponsStorePopUpForStoreId:self.store.storeID];
            [[WMLNavigationService sharedService] navigateToURL:URL];
            self.couponsListViewController.extendedLayoutIncludesOpaqueBars = YES;
            break;
        }
        case WMLStorePageSaved: {
            NSURL *URL = [[WMLRouteBuilder sharedRouteBuilder] savedListURLForStore:self.store.storeID];
            [[WMLNavigationService sharedService] navigateToURL:URL];
            self.savedProductsViewController.extendedLayoutIncludesOpaqueBars = YES;
            break;
        }
    }
    [self.collapsingManager setHidden:NO animated:NO completion:nil];
}

@end

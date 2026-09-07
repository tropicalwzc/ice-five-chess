//
//  ice_five_chessUITests.m
//  ice five chessUITests
//
//  Created by 王子诚 on 2019/5/11.
//  Copyright © 2019 王子诚. All rights reserved.
//

#import <XCTest/XCTest.h>

@interface ice_five_chessUITests : XCTestCase
@property (nonatomic) XCUIApplication *app;
@end

@implementation ice_five_chessUITests

- (void)setUp {
    // Put setup code here. This method is called before the invocation of each test method in the class.

    // In UI tests it is usually best to stop immediately when a failure occurs.
    self.continueAfterFailure = NO;

    // UI tests must launch the application that they test. Doing this in setup will make sure it happens for each test method.
    self.app = [[XCUIApplication alloc] init];
    self.app.launchArguments = @[@"-AppleLanguages", @"(en)", @"-AppleLocale", @"en_US"];
    XCUIDevice.sharedDevice.orientation = UIDeviceOrientationPortrait;
    [self.app launch];

    // In UI tests it’s important to set the initial state - such as interface orientation - required for your tests before they run. The setUp method is a good place to do this.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
}

- (void)reveal:(XCUIElement *)element {
    for (int i = 0; i < 4 && !element.hittable && self.app.scrollViews.count > 0; i++) [self.app.scrollViews.firstMatch swipeUp];
    XCTAssertTrue(element.hittable);
}

- (void)scrollToTopIfNeeded {
    if (self.app.scrollViews.count > 0) {
        [self.app.scrollViews.firstMatch swipeDown];
        [self.app.scrollViews.firstMatch swipeDown];
    }
}

- (void)assertPhoneControlsFit {
    if (UIDevice.currentDevice.userInterfaceIdiom != UIUserInterfaceIdiomPhone) return;
    // Accessibility frames round to physical pixels, especially after rotation.
    CGFloat pixelTolerance = 1.0 / UIScreen.mainScreen.scale + 0.001;
    XCTAssertEqual(self.app.scrollViews.count, 0, @"The standard iPhone game must fit without scrolling.");
    for (NSString *identifier in @[@"how-to-play", @"undo-turn", @"show-hints", @"new-game", @"difficulty"]) {
        XCUIElement *button = self.app.buttons[identifier];
        XCTAssertTrue(button.exists, @"%@ exists", identifier);
        XCTAssertTrue(CGRectContainsRect(self.app.frame, button.frame), @"%@ fits inside the screen: %@", identifier, NSStringFromCGRect(button.frame));
        XCTAssertGreaterThanOrEqual(CGRectGetHeight(button.frame) + pixelTolerance, 44, @"%@ keeps its touch target", identifier);
    }
    XCTAssertGreaterThanOrEqual(CGRectGetHeight(self.app.buttons[@"new-game"].frame) + pixelTolerance, 52);
    XCTAssertGreaterThan(CGRectGetWidth(self.app.buttons[@"new-game"].frame), CGRectGetWidth(self.app.buttons[@"undo-turn"].frame) * 1.8);
}

- (void)capture:(NSString *)name {
    XCTAttachment *image = [XCTAttachment attachmentWithScreenshot:XCUIScreen.mainScreen.screenshot];
    image.name = name;
    image.lifetime = XCTAttachmentLifetimeKeepAlways;
    [self addAttachment:image];
}

- (void)assertExpandedBoardAndEdgeSelection {
    XCUIElement *board = self.app.otherElements[@"chess-board"];
    XCTAssertTrue(board.exists);
    XCTAssertEqual(board.buttons.count, 225);
    // The grid now occupies over 90% of the board width, with no coordinate gutter.
    CGFloat gridWidth = CGRectGetMidX(self.app.buttons[@"cell-224"].frame) - CGRectGetMidX(self.app.buttons[@"cell-0"].frame);
    XCTAssertGreaterThan(gridWidth / CGRectGetWidth(board.frame), 0.90);
    for (NSString *identifier in @[@"cell-0", @"cell-14", @"cell-210", @"cell-224"]) {
        XCUIElement *cell = self.app.buttons[identifier];
        XCTAssertTrue(CGRectContainsRect(board.frame, cell.frame));
        [cell tap];
        XCTAssertEqualObjects(cell.value, @"Selected");
    }
    XCTAssertEqualObjects(self.app.staticTexts[@"move-count"].label, @"0 moves");
}

- (void)testDescriptiveControlsGameplayAndRotation {
    XCTAssertTrue([self.app.buttons[@"how-to-play"] waitForExistenceWithTimeout:10]);
    [self assertPhoneControlsFit];
    [self.app.buttons[@"how-to-play"] tap];
    XCTAssertTrue([self.app.navigationBars[@"How to Play"] waitForExistenceWithTimeout:5]);
    [self.app.buttons[@"Done"] tap];

    [self reveal:self.app.buttons[@"new-game"]];
    [self.app.buttons[@"new-game"] tap];
    XCTAssertTrue(self.app.buttons[@"start-new-game"].hittable);
    [self capture:@"Centered English new game button"];
    [self.app.buttons[@"start-new-game"] tap];
    [self scrollToTopIfNeeded];
    XCTAssertFalse(self.app.buttons[@"place-stone"].exists);
    [self assertExpandedBoardAndEdgeSelection];
    [self.app.buttons[@"cell-112"] tap];
    XCTAssertEqualObjects(self.app.staticTexts[@"move-count"].label, @"0 moves");
    XCTAssertEqualObjects(self.app.buttons[@"cell-112"].value, @"Selected");
    [self.app.buttons[@"cell-113"] tap];
    XCTAssertEqualObjects(self.app.staticTexts[@"move-count"].label, @"0 moves");
    XCTAssertEqualObjects(self.app.buttons[@"cell-113"].value, @"Selected");
    XCTAssertNotEqualObjects(self.app.buttons[@"cell-112"].value, @"Selected");
    [self.app.buttons[@"cell-113"] tap];
    NSPredicate *replied = [NSPredicate predicateWithFormat:@"label == '2 moves'"];
    [self expectationForPredicate:replied evaluatedWithObject:self.app.staticTexts[@"move-count"] handler:nil];
    [self waitForExpectationsWithTimeout:30 handler:nil];

    [self.app terminate];
    [self.app launch];
    XCTAssertTrue([self.app.staticTexts[@"move-count"] waitForExistenceWithTimeout:10]);
    XCTAssertEqualObjects(self.app.staticTexts[@"move-count"].label, @"2 moves");
    [self reveal:self.app.buttons[@"show-hints"]];
    [self.app.buttons[@"show-hints"] tap];
    [self expectationForPredicate:[NSPredicate predicateWithFormat:@"label CONTAINS 'Hide Hints'"]
             evaluatedWithObject:self.app.buttons[@"show-hints"] handler:nil];
    [self waitForExpectationsWithTimeout:30 handler:nil];
    XCTAssertEqualObjects(self.app.staticTexts[@"move-count"].label, @"2 moves");
    [self.app.buttons[@"show-hints"] tap];
    [self.app.buttons[@"undo-turn"] tap];
    XCTAssertEqualObjects(self.app.staticTexts[@"move-count"].label, @"0 moves");
    [self scrollToTopIfNeeded];
    [self.app.buttons[@"cell-112"] doubleTap];
    [self expectationForPredicate:replied evaluatedWithObject:self.app.staticTexts[@"move-count"] handler:nil];
    [self waitForExpectationsWithTimeout:30 handler:nil];
    XCTAssertTrue([self.app.buttons[@"cell-112"].label containsString:@"black stone"]);
    [self capture:@"Opaque stones after double-tap"];
    [self reveal:self.app.buttons[@"undo-turn"]];
    [self.app.buttons[@"undo-turn"] tap];
    [self scrollToTopIfNeeded];

    XCUIDevice.sharedDevice.orientation = UIDeviceOrientationLandscapeLeft;
    [self expectationForPredicate:[NSPredicate predicateWithBlock:^BOOL(XCUIApplication *app, NSDictionary *bindings) {
        return CGRectGetWidth(app.frame) > CGRectGetHeight(app.frame);
    }] evaluatedWithObject:self.app handler:nil];
    [self waitForExpectationsWithTimeout:10 handler:nil];
    XCTAssertTrue([self.app.buttons[@"cell-112"] waitForExistenceWithTimeout:5]);
    [self assertExpandedBoardAndEdgeSelection];
    [self.app.buttons[@"cell-112"] tap];
    XCTAssertEqualObjects(self.app.buttons[@"cell-112"].value, @"Selected");
    XCTAssertFalse(self.app.buttons[@"place-stone"].exists);
    [self assertPhoneControlsFit];
    [self capture:@"Landscape SwiftUI board"];
}

- (void)testChineseHelp {
    [self.app terminate];
    self.app.launchArguments = @[@"-AppleLanguages", @"(zh-Hans)", @"-AppleLocale", @"zh_CN"];
    [self.app launch];
    XCUIElement *help = self.app.buttons[@"how-to-play"];
    XCTAssertTrue([help waitForExistenceWithTimeout:10]);
    XCTAssertTrue([help.label containsString:@"玩法说明"]);
    [help tap];
    XCTAssertTrue([self.app.navigationBars[@"玩法说明"] waitForExistenceWithTimeout:5]);
    [self capture:@"Chinese instructions"];
    [self.app.buttons[@"完成"] tap];
    [self reveal:self.app.buttons[@"new-game"]];
    [self.app.buttons[@"new-game"] tap];
    XCUIElement *start = self.app.buttons[@"start-new-game"];
    XCTAssertTrue([start waitForExistenceWithTimeout:5]);
    XCTAssertEqualObjects(start.label, @"开始新棋局");
    XCTAssertTrue(start.hittable);
    [self capture:@"Centered Chinese new game button"];
}

@end

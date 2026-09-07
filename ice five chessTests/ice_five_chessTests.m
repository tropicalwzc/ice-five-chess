//
//  ice_five_chessTests.m
//  ice five chessTests
//
//  Created by 王子诚 on 2019/5/11.
//  Copyright © 2019 王子诚. All rights reserved.
//

#import <XCTest/XCTest.h>
#import "../ice five chess/ChessEngine.h"

@interface ice_five_chessTests : XCTestCase
@property (nonatomic) NSURL *directory;
@property (nonatomic) NSURL *saveURL;
@end

@implementation ice_five_chessTests

- (void)setUp {
    self.directory = [[NSURL fileURLWithPath:NSTemporaryDirectory()] URLByAppendingPathComponent:NSUUID.UUID.UUIDString isDirectory:YES];
    [[NSFileManager defaultManager] createDirectoryAtURL:self.directory withIntermediateDirectories:YES attributes:nil error:nil];
    self.saveURL = [self.directory URLByAppendingPathComponent:@"game.plist"];
}

- (void)tearDown {
    [[NSFileManager defaultManager] removeItemAtURL:self.directory error:nil];
}

- (ChessEngine *)game { return [[ChessEngine alloc] initWithSaveURL:self.saveURL]; }

- (void)testDefaultFourStarsAndRestoredDifficulty {
    ChessEngine *game = self.game;
    XCTAssertEqual(game.difficulty, 3); // Four-star persisted value.
    game.difficulty = 4; // A user's saved five-star selection still takes precedence.
    [game save];
    XCTAssertEqual(self.game.difficulty, 4);
}

- (void)testMoveValidationAndSavedInterruptedTurn {
    ChessEngine *game = self.game;
    XCTAssertFalse([game placeAt:-1]);
    XCTAssertFalse([game placeAt:225]);
    XCTAssertTrue([game placeAt:112]);
    XCTAssertTrue(game.computerTurn);
    XCTAssertFalse([game placeAt:113]);
    ChessEngine *restored = self.game;
    XCTAssertEqualObjects(restored.moves, (@[@112]));
    XCTAssertTrue(restored.computerTurn);
    XCTAssertEqual(restored.cells[112].intValue, 1);
}

- (void)testWhiteOpeningAndUndoPreservePlayerTurn {
    ChessEngine *game = self.game;
    [game startWithPlayer:-1 forbidden:YES];
    XCTAssertEqualObjects(game.moves, (@[@112]));
    XCTAssertFalse(game.computerTurn);
    XCTAssertTrue([game placeAt:113]);
    [game undo];
    XCTAssertEqualObjects(game.moves, (@[@112]));
    XCTAssertFalse(game.computerTurn);
    XCTAssertTrue(game.forbidden);
    XCTAssertEqual(self.game.player, -1);
}

- (void)testComputerReplyAndUndo {
    ChessEngine *game = self.game;
    game.difficulty = 2;
    XCTAssertTrue([game placeAt:112]);
    XCTestExpectation *done = [self expectationWithDescription:@"computer reply"];
    [game analyzeWithCompletion:^{
        XCTAssertTrue(NSThread.isMainThread);
        XCTAssertEqual(game.moves.count, 2);
        XCTAssertFalse(game.computerTurn);
        [game undo];
        XCTAssertEqual(game.moves.count, 0);
        [done fulfill];
    }];
    [self waitForExpectationsWithTimeout:20 handler:nil];
}

- (void)testHintsDoNotMutateGame {
    ChessEngine *game = self.game;
    [game startWithPlayer:-1 forbidden:NO];
    NSArray *before = game.cells;
    XCTestExpectation *done = [self expectationWithDescription:@"hints"];
    [game hintsWithCompletion:^(NSArray<NSNumber *> *hints) {
        XCTAssertEqual(hints.count, 225);
        XCTAssertEqualObjects(game.cells, before);
        XCTAssertEqualObjects(game.moves, (@[@112]));
        XCTAssertFalse(game.computerTurn);
        [done fulfill];
    }];
    [self waitForExpectationsWithTimeout:20 handler:nil];
}

- (void)testLegacySaveMigration {
    NSMutableString *board = [NSMutableString string];
    for (int i = 0; i < 225; i++) [board appendString:i == 112 ? @"1" : i == 113 ? @"/" : @"0"];
    NSDictionary *files = @{@"autosave_player_main": board, @"autosave_player_stack": @"HHHI",
        @"autosave_player_ch": @"1", @"autosave_player_df": @"4", @"autosave_player_ban": @"1"};
    [files enumerateKeysAndObjectsUsingBlock:^(NSString *name, NSString *value, BOOL *stop) {
        [value writeToURL:[self.directory URLByAppendingPathComponent:name] atomically:YES encoding:NSUTF8StringEncoding error:nil];
    }];
    ChessEngine *game = self.game;
    XCTAssertEqualObjects(game.moves, (@[@112, @113]));
    XCTAssertEqual(game.difficulty, 4);
    XCTAssertTrue(game.forbidden);
    XCTAssertEqualObjects(self.game.cells, game.cells);
}

- (void)testRejectsCorruptSave {
    [@{@"version": @1, @"moves": @[@112, @112], @"player": @1,
       @"difficulty": @0, @"forbidden": @NO} writeToURL:self.saveURL atomically:YES];
    XCTAssertEqual(self.game.moves.count, 0);
}

- (void)testFinishedGameRestoresAndCanBeUndone {
    [@{@"version": @1, @"moves": @[@0, @15, @1, @16, @2, @17, @3, @18, @4],
       @"player": @1, @"difficulty": @0, @"forbidden": @NO, @"winner": @1}
        writeToURL:self.saveURL atomically:YES];
    ChessEngine *game = self.game;
    XCTAssertEqual(game.winner, 1);
    XCTAssertTrue(game.finished);
    XCTAssertFalse([game placeAt:5]);
    [game undo];
    XCTAssertFalse(game.finished);
    XCTAssertFalse(game.computerTurn);
    XCTAssertEqual(game.moves.count, 8);
}

@end

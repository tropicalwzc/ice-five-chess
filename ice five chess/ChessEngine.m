#import "ChessEngine.h"
#import "doublethree.h"

@interface ChessEngine ()
@property (nonatomic, strong) doublethree *engine;
@property (nonatomic, strong) NSURL *saveURL;
@property (nonatomic, readwrite) NSInteger player;
@property (nonatomic, readwrite) BOOL forbidden;
@property (nonatomic) BOOL working;
@property (nonatomic) NSInteger restoredWinner;
@end

@implementation ChessEngine

- (instancetype)initWithSaveURL:(NSURL *)url {
    if ((self = [super init])) {
        _saveURL = url;
        _difficulty = 3; // Default: 4 stars. Persisted mapping: 0=3, 1=2, 2=1, 3=4, 4=5 stars.
        [self startWithPlayer:1 forbidden:NO];
        NSDictionary *saved = [NSDictionary dictionaryWithContentsOfURL:url];
        if (saved != nil) {
            [self restore:saved];
        } else {
            [self migrateLegacySave];
        }
    }
    return self;
}

- (NSArray<NSNumber *> *)cells {
    int board[15][15] = {};
    [self.engine export_current_board:board];
    NSMutableArray *values = [NSMutableArray arrayWithCapacity:225];
    for (int x = 0; x < 15; x++)
        for (int y = 0; y < 15; y++) [values addObject:@(board[x][y])];
    return values;
}

- (NSArray<NSNumber *> *)moves {
    int stack[225][2] = {};
    int count = [self.engine export_stack:stack];
    NSMutableArray *values = [NSMutableArray arrayWithCapacity:count];
    for (int i = 0; i < count; i++) [values addObject:@(stack[i][0] * 15 + stack[i][1])];
    return values;
}

- (NSInteger)winner { return self.restoredWinner ?: self.engine.win_state; }
- (BOOL)finished { return self.winner != 0 || self.moves.count == 225; }
- (BOOL)computerTurn {
    NSInteger next = self.moves.count % 2 == 0 ? 1 : -1;
    return !self.finished && next != self.player;
}

- (void)startWithPlayer:(NSInteger)player forbidden:(BOOL)forbidden {
    if (self.working) return;
    self.player = player == -1 ? -1 : 1;
    self.forbidden = forbidden;
    self.restoredWinner = 0;
    self.engine = [[doublethree alloc] init];
    [self.engine set_banmode:forbidden ? 1 : 0];
    if (self.player == -1) [self.engine add_a_chess:7 pl_y:7 mode:1];
}

- (BOOL)placeAt:(NSInteger)index {
    if (self.working || self.finished || self.computerTurn || index < 0 || index >= 225) return NO;
    if (self.cells[index].intValue != 0) return NO;
    int x = (int)index / 15, y = (int)index % 15;
    if (self.player == 1 && self.forbidden && [self.engine banned_point:x j:y] == 1) return NO;
    [self.engine add_a_chess:x pl_y:y mode:(int)self.player];
    [self save];
    return YES;
}

- (void)undo {
    if (self.working) return;
    NSMutableArray *moves = [self.moves mutableCopy];
    NSInteger minimum = self.player == -1 ? 1 : 0;
    if ((NSInteger)moves.count <= minimum) return;
    // A finished game can end immediately after the human's move.
    [moves removeLastObject];
    while ((NSInteger)moves.count > minimum &&
           (moves.count % 2 == 0 ? 1 : -1) != self.player) [moves removeLastObject];
    self.engine = [self engineForMoves:moves];
    self.restoredWinner = 0;
    [self save];
}

- (doublethree *)engineForMoves:(NSArray<NSNumber *> *)moves {
    doublethree *copy = [[doublethree alloc] init];
    [copy set_banmode:self.forbidden ? 1 : 0];
    for (NSUInteger i = 0; i < moves.count; i++) {
        int index = moves[i].intValue;
        [copy add_a_chess:index / 15 pl_y:index % 15 mode:i % 2 == 0 ? 1 : -1];
    }
    return copy;
}

- (void)runWork:(void (^)(void))work {
    NSThread *thread = [[NSThread alloc] initWithBlock:^{ @autoreleasepool { work(); } }];
    thread.stackSize = 4 * 1024 * 1024;
    thread.qualityOfService = NSQualityOfServiceUserInitiated;
    [thread start];
}

- (void)analyzeWithCompletion:(void (^)(void))completion {
    if (self.working || !self.computerTurn) { completion(); return; }
    self.working = YES;
    doublethree *copy = [self engineForMoves:self.moves];
    NSInteger difficulty = self.difficulty;
    int color = -(int)self.player;
    [self runWork:^{
        switch (difficulty) {
            case 1: [copy easy_analysisboard:color]; break;
            case 2: [copy egg_analysisboard:color]; break;
            case 3: [copy four_star_analysisboard:color]; break;
            case 4: [copy five_star_analysisboard:color]; break;
            default: [copy harsh_analysisboard:color]; break;
        }
        dispatch_async(dispatch_get_main_queue(), ^{
            self.engine = copy;
            self.working = NO;
            [self save];
            completion();
        });
    }];
}

- (void)hintsWithCompletion:(void (^)(NSArray<NSNumber *> *))completion {
    if (self.working || self.finished || self.computerTurn) { completion(@[]); return; }
    self.working = YES;
    doublethree *copy = [self engineForMoves:self.moves];
    [self runWork:^{
        int hints[15][15] = {};
        // The legacy continuation needs an existing last move.
        int stack[225][2] = {};
        if ([copy export_stack:stack] == 0) hints[7][7] = 1;
        else [copy teaching_current_step:hints];
        NSMutableArray *values = [NSMutableArray arrayWithCapacity:225];
        for (int x = 0; x < 15; x++)
            for (int y = 0; y < 15; y++) [values addObject:@(hints[x][y])];
        dispatch_async(dispatch_get_main_queue(), ^{
            self.working = NO;
            completion(values);
        });
    }];
}

- (void)save {
    NSDictionary *state = @{@"version": @1, @"moves": self.moves,
        @"player": @(self.player), @"difficulty": @(self.difficulty),
        @"forbidden": @(self.forbidden), @"winner": @(self.winner)};
    [state writeToURL:self.saveURL atomically:YES];
}

- (BOOL)restore:(NSDictionary *)saved {
    if (![saved isKindOfClass:NSDictionary.class] || ![saved[@"version"] isEqual:@1]) return NO;
    NSArray *moves = saved[@"moves"];
    NSNumber *player = saved[@"player"], *difficulty = saved[@"difficulty"], *forbidden = saved[@"forbidden"];
    if (![moves isKindOfClass:NSArray.class] || moves.count > 225 ||
        ![player isKindOfClass:NSNumber.class] || ![difficulty isKindOfClass:NSNumber.class] ||
        ![forbidden isKindOfClass:NSNumber.class] || abs(player.intValue) != 1 ||
        difficulty.integerValue < 0 || difficulty.integerValue > 4) return NO;
    NSMutableSet *seen = [NSMutableSet set];
    for (id value in moves) {
        if (![value isKindOfClass:NSNumber.class] || [value integerValue] < 0 ||
            [value integerValue] >= 225 || [seen containsObject:value]) return NO;
        [seen addObject:value];
    }
    self.player = player.integerValue;
    self.difficulty = difficulty.integerValue;
    self.forbidden = forbidden.boolValue;
    self.engine = [self engineForMoves:moves];
    NSNumber *winner = saved[@"winner"];
    self.restoredWinner = [winner isKindOfClass:NSNumber.class] && abs(winner.intValue) == 1 ? winner.integerValue : 0;
    return YES;
}

- (void)migrateLegacySave {
    NSURL *directory = [self.saveURL URLByDeletingLastPathComponent];
    NSString *(^read)(NSString *) = ^NSString *(NSString *name) {
        return [NSString stringWithContentsOfURL:[directory URLByAppendingPathComponent:name]
                                       encoding:NSUTF8StringEncoding error:nil];
    };
    NSString *board = read(@"autosave_player_main");
    NSString *stack = read(@"autosave_player_stack");
    if (board.length != 225 || stack == nil || stack.length > 450 || stack.length % 2 != 0) return;
    NSMutableArray *moves = [NSMutableArray array];
    NSMutableString *rebuilt = [NSMutableString stringWithCapacity:225];
    for (int i = 0; i < 225; i++) [rebuilt appendString:@"0"];
    for (NSUInteger i = 0; i < stack.length; i += 2) {
        NSInteger x = [stack characterAtIndex:i] - 'A', y = [stack characterAtIndex:i + 1] - 'A';
        if (x < 0 || x >= 15 || y < 0 || y >= 15) return;
        NSInteger index = x * 15 + y;
        if ([rebuilt characterAtIndex:index] != '0') return;
        [moves addObject:@(index)];
        [rebuilt replaceCharactersInRange:NSMakeRange(index, 1) withString:i % 4 == 0 ? @"1" : @"/"];
    }
    if (![board isEqualToString:rebuilt]) return;
    NSDictionary *state = @{@"version": @1, @"moves": moves,
        @"player": @(read(@"autosave_player_ch").integerValue == -1 ? -1 : 1),
        @"difficulty": @(MAX(0, MIN(4, read(@"autosave_player_df").integerValue))),
        @"forbidden": @(read(@"autosave_player_ban").integerValue == 1)};
    if ([self restore:state]) [self save];
}
@end

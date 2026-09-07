#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/// Main-thread game state. Analysis runs on an isolated engine with a large stack.
@interface ChessEngine : NSObject
@property (nonatomic, readonly) NSArray<NSNumber *> *cells;
@property (nonatomic, readonly) NSArray<NSNumber *> *moves;
@property (nonatomic, readonly) NSInteger winner;
@property (nonatomic, readonly) NSInteger player;
@property (nonatomic) NSInteger difficulty;
@property (nonatomic, readonly) BOOL forbidden;
@property (nonatomic, readonly) BOOL computerTurn;
@property (nonatomic, readonly) BOOL finished;
- (instancetype)initWithSaveURL:(NSURL *)url;
- (void)startWithPlayer:(NSInteger)player forbidden:(BOOL)forbidden;
- (BOOL)placeAt:(NSInteger)index;
- (void)undo;
- (void)analyzeWithCompletion:(void (^)(void))completion;
- (void)hintsWithCompletion:(void (^)(NSArray<NSNumber *> *))completion;
- (void)save;
@end

NS_ASSUME_NONNULL_END

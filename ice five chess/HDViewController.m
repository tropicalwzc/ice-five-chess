//
//  ViewController.m
//  ice five chess
//
//  Created by 王子诚 on 2019/5/11.
//  Copyright © 2019 王子诚. All rights reserved.
//

#import "HDViewController.h"
#import <QuartzCore/QuartzCore.h>

// Keep persisted values compatible with earlier releases:
// 0 = legacy three-star, 1 = two-star, 2 = one-star, 3 = proof-guided four-star.
static int fc_difficulty_for_segment(NSInteger segment)
{
    static const int values[5] = {4, 3, 0, 1, 2};
    return segment >= 0 && segment < 5 ? values[segment] : 0;
}

static NSInteger fc_segment_for_difficulty(int difficulty)
{
    static const NSInteger segments[5] = {2, 3, 4, 1, 0};
    return difficulty >= 0 && difficulty < 5 ? segments[difficulty] : 2;
}

static const NSInteger FCPreviewDotTag = 0x5FC0;
static const NSInteger FCStoneViewTag = 0x5FC1;
static const NSInteger FCRecentMoveRingTag = 0x5FC2;

@interface ViewController_m ()
@property (weak, nonatomic) IBOutlet UIBarButtonItem *player_chess_choice;
@property (weak, nonatomic) IBOutlet UISegmentedControl *player_choice_seg;
@property (weak, nonatomic) IBOutlet UISegmentedControl *difficulty_choice_seg;
@property (weak, nonatomic) IBOutlet UISegmentedControl *banbar;
@property (weak, nonatomic) IBOutlet UILabel *tech_texter;
@property (weak, nonatomic) IBOutlet UISegmentedControl *ban_choice;
@property (weak, nonatomic) IBOutlet UILabel *sudoback;
@property (weak, nonatomic) IBOutlet UIActivityIndicatorView* rotater;
@property (weak, nonatomic) IBOutlet UIToolbar *bottom_toolbar;
@property (strong, nonatomic) UIView *board_surface;
@property (strong, nonatomic) CAGradientLayer *board_gradient_layer;
@property (strong, nonatomic) CAShapeLayer *board_grid_layer;
@property (strong, nonatomic) CAShapeLayer *board_star_layer;
@property (strong, nonatomic) CAShapeLayer *board_border_layer;
@property (strong, nonatomic) UIView *analysis_loading_panel;
@property (strong, nonatomic) UILabel *analysis_status_label;
@property (strong, nonatomic) UIActivityIndicatorView *analysis_loading_spinner;
@property (strong, nonatomic) NSOperationQueue *analysis_queue;
@property (assign, nonatomic) NSUInteger analysis_request_id;
@property (assign, nonatomic) CGFloat board_cell_size;
@property (assign, nonatomic) CGPoint board_grid_origin;
@property (assign, nonatomic) BOOL restart_pending;
@property (assign, nonatomic) int pending_player_chess_id;
@property (assign, nonatomic) int pending_ban_mode;
@property (assign, nonatomic) int pending_difficulty;
@property (assign, nonatomic) BOOL drawing_teacher_overlay;
@property (strong, nonatomic) NSData *teacher_overlay_data;
- (void)apply_pending_game_settings;
- (void)finish_analysis_and_apply_pending_restart;
- (void)perform_restart_now;
- (void)enqueue_analysis_for_x:(int)x
                              y:(int)y
                         player:(int)player
                     difficulty:(int)difficulty
                      requestID:(NSUInteger)requestID;
- (void)try_next_step_for_x:(int)x
                           y:(int)y
                      player:(int)player
                  difficulty:(int)difficulty
                   requestID:(NSUInteger)requestID;
- (void)analysis_next_step_for_player:(int)player difficulty:(int)difficulty;
- (void)setup_analysis_loading_indicator;
- (void)layout_analysis_loading_indicator;
- (void)update_analysis_loading_appearance;
- (void)hide_analysis_loading;
- (void)render_teacher_overlay;
- (void)update_board_surface_geometry;
- (void)layout_board;
@end

@implementation ViewController_m

- (CGRect)board_grid_frame
{
    CGRect bounds = self.view.bounds;
    CGFloat width = CGRectGetWidth(bounds);
    CGFloat height = CGRectGetHeight(bounds);
    if (width <= 1.0 || height <= 1.0) {
        width = MAX(1.0, (CGFloat)ScreenWidth);
        height = MAX(1.0, (CGFloat)ScreenHeight);
        bounds = CGRectMake(0.0, 0.0, width, height);
    }

    // The board lives in the space between the status text and the bottom
    // toolbar. Using the view's bounds (instead of UIScreen's portrait
    // bounds) also keeps the layout correct after an iPad rotation.
    CGRect contentFrame = UIEdgeInsetsInsetRect(bounds, self.view.safeAreaInsets);
    CGFloat top = CGRectGetMinY(contentFrame) + 96.0;
    if (_tech_texter != nil && _tech_texter.superview != nil) {
        CGRect infoFrame = [_tech_texter.superview convertRect:_tech_texter.frame toView:self.view];
        top = MAX(top, CGRectGetMaxY(infoFrame) + 14.0);
    }

    CGFloat bottom = CGRectGetMaxY(contentFrame) - 55.0 - 16.0;
    if (_bottom_toolbar != nil && _bottom_toolbar.superview != nil) {
        CGRect toolbarFrame = [_bottom_toolbar.superview convertRect:_bottom_toolbar.frame toView:self.view];
        if (CGRectGetHeight(toolbarFrame) > 1.0) {
            bottom = MIN(bottom, CGRectGetMinY(toolbarFrame) - 16.0);
        }
    }

    if (bottom <= top) {
        bottom = CGRectGetMaxY(contentFrame) - 16.0;
        top = MIN(top, bottom - 120.0);
    }

    CGFloat horizontalInset = MIN(72.0, MAX(24.0, width * 0.05));
    CGFloat availableWidth = MAX(1.0, CGRectGetWidth(contentFrame) - horizontalInset * 2.0);
    CGFloat availableHeight = MAX(1.0, bottom - top);
    CGFloat boardSize = MIN(availableWidth, availableHeight);
    CGFloat cell = MAX(1.0, boardSize / 15.0);
    boardSize = cell * 15.0;
    CGFloat originX = CGRectGetMidX(contentFrame) - boardSize * 0.5;
    CGFloat originY = top + (availableHeight - boardSize) * 0.5;

    self.board_cell_size = cell;
    self.board_grid_origin = CGPointMake(originX, originY);
    return CGRectMake(originX, originY, boardSize, boardSize);
}

- (void)setup_board_surface
{
    CGRect gridFrame = [self board_grid_frame];
    UIView *surface = [[UIView alloc] initWithFrame:gridFrame];
    surface.userInteractionEnabled = NO;
    surface.layer.cornerRadius = MIN(16.0, self.board_cell_size * 0.45);
    surface.layer.shadowColor = [UIColor blackColor].CGColor;
    surface.layer.shadowOpacity = 0.20;
    surface.layer.shadowRadius = 12.0;
    surface.layer.shadowOffset = CGSizeMake(0.0, 5.0);
    self.board_surface = surface;

    CAGradientLayer *gradient = [CAGradientLayer layer];
    gradient.frame = surface.bounds;
    gradient.cornerRadius = surface.layer.cornerRadius;
    gradient.masksToBounds = YES;
    gradient.startPoint = CGPointMake(0.0, 0.0);
    gradient.endPoint = CGPointMake(1.0, 1.0);
    self.board_gradient_layer = gradient;
    [surface.layer addSublayer:gradient];

    CAShapeLayer *gridLayer = [CAShapeLayer layer];
    gridLayer.fillColor = UIColor.clearColor.CGColor;
    gridLayer.contentsScale = UIScreen.mainScreen.scale;
    self.board_grid_layer = gridLayer;
    [surface.layer addSublayer:gridLayer];

    CAShapeLayer *starLayer = [CAShapeLayer layer];
    starLayer.contentsScale = UIScreen.mainScreen.scale;
    self.board_star_layer = starLayer;
    [surface.layer addSublayer:starLayer];

    CAShapeLayer *borderLayer = [CAShapeLayer layer];
    borderLayer.fillColor = UIColor.clearColor.CGColor;
    borderLayer.contentsScale = UIScreen.mainScreen.scale;
    self.board_border_layer = borderLayer;
    [surface.layer addSublayer:borderLayer];

    if (_sudoback != nil) {
        [self.view insertSubview:surface aboveSubview:_sudoback];
    } else {
        [self.view insertSubview:surface atIndex:0];
    }
    [self update_board_surface_geometry];
    [self update_board_surface_appearance];
}

- (void)update_board_surface_geometry
{
    if (self.board_surface == nil) {
        return;
    }

    CGFloat cell = self.board_cell_size;
    CGFloat lineOffset = cell * 0.5;
    self.board_surface.layer.cornerRadius = MIN(16.0, cell * 0.45);
    self.board_gradient_layer.frame = self.board_surface.bounds;
    self.board_gradient_layer.cornerRadius = self.board_surface.layer.cornerRadius;

    UIBezierPath *gridPath = [UIBezierPath bezierPath];
    for (NSInteger index = 0; index < 15; index++) {
        CGFloat offset = lineOffset + index * cell;
        [gridPath moveToPoint:CGPointMake(offset, lineOffset)];
        [gridPath addLineToPoint:CGPointMake(offset, CGRectGetHeight(self.board_surface.bounds) - lineOffset)];
        [gridPath moveToPoint:CGPointMake(lineOffset, offset)];
        [gridPath addLineToPoint:CGPointMake(CGRectGetWidth(self.board_surface.bounds) - lineOffset, offset)];
    }
    self.board_grid_layer.path = gridPath.CGPath;
    self.board_grid_layer.lineWidth = MAX(0.8, cell * 0.035);

    UIBezierPath *starPath = [UIBezierPath bezierPath];
    NSArray<NSNumber *> *starIndexes = @[@3, @7, @11];
    CGFloat starRadius = MAX(2.4, cell * 0.10);
    for (NSNumber *xNumber in starIndexes) {
        for (NSNumber *yNumber in starIndexes) {
            CGFloat x = lineOffset + xNumber.integerValue * cell;
            CGFloat y = lineOffset + yNumber.integerValue * cell;
            [starPath moveToPoint:CGPointMake(x + starRadius, y)];
            [starPath addArcWithCenter:CGPointMake(x, y)
                                radius:starRadius
                            startAngle:0.0
                              endAngle:(CGFloat)(M_PI * 2.0)
                             clockwise:YES];
        }
    }
    self.board_star_layer.path = starPath.CGPath;

    UIBezierPath *borderPath = [UIBezierPath bezierPathWithRoundedRect:CGRectInset(self.board_surface.bounds, lineOffset, lineOffset)
                                                                  cornerRadius:MIN(10.0, cell * 0.30)];
    self.board_border_layer.path = borderPath.CGPath;
    self.board_border_layer.lineWidth = MAX(1.4, cell * 0.07);
}

- (void)layout_board
{
    if (self.board_surface == nil) {
        return;
    }

    CGRect oldSurfaceFrame = self.board_surface.frame;
    CGRect gridFrame = [self board_grid_frame];
    BOOL geometryChanged = !CGRectEqualToRect(oldSurfaceFrame, gridFrame);
    self.board_surface.frame = gridFrame;
    [self update_board_surface_geometry];

    for (NSInteger x = 0; x < 15; x++) {
        for (NSInteger y = 0; y < 15; y++) {
            UIButton *button = chess_map[x][y];
            if (button == nil) {
                continue;
            }
            button.frame = CGRectMake(CGRectGetMinX(gridFrame) + x * self.board_cell_size,
                                      CGRectGetMinY(gridFrame) + y * self.board_cell_size,
                                      self.board_cell_size,
                                      self.board_cell_size);
        }
    }

    if (geometryChanged) {
        [self flush_chess_map_according_to:map_state];
        if (focus_has_been_selected && focus_x >= 0 && focus_x < 15 && focus_y >= 0 && focus_y < 15) {
            [self set_op_focus_sign];
        }
    }
}

- (void)update_board_surface_appearance
{
    UIColor *topColor = is_darkmode
        ? [UIColor colorWithRed:0.24 green:0.15 blue:0.08 alpha:1.0]
        : [UIColor colorWithRed:0.94 green:0.76 blue:0.49 alpha:1.0];
    UIColor *bottomColor = is_darkmode
        ? [UIColor colorWithRed:0.13 green:0.08 blue:0.04 alpha:1.0]
        : [UIColor colorWithRed:0.76 green:0.52 blue:0.27 alpha:1.0];
    UIColor *lineColor = is_darkmode
        ? [UIColor colorWithWhite:0.96 alpha:0.70]
        : [UIColor colorWithRed:0.20 green:0.12 blue:0.05 alpha:0.78];
    UIColor *starColor = is_darkmode
        ? [UIColor colorWithWhite:1.0 alpha:0.82]
        : [UIColor colorWithRed:0.16 green:0.09 blue:0.03 alpha:0.84];

    self.board_surface.backgroundColor = topColor;
    self.board_gradient_layer.colors = @[(id)topColor.CGColor, (id)bottomColor.CGColor];
    self.board_grid_layer.strokeColor = lineColor.CGColor;
    self.board_star_layer.fillColor = starColor.CGColor;
    self.board_border_layer.strokeColor = lineColor.CGColor;
}

- (void)remove_preview_dot_from_button:(UIButton *)button
{
    if (button == nil) {
        return;
    }
    [[button viewWithTag:FCPreviewDotTag] removeFromSuperview];
}

- (void)remove_stone_from_button:(UIButton *)button
{
    if (button == nil) {
        return;
    }
    [[button viewWithTag:FCStoneViewTag] removeFromSuperview];
}

- (void)remove_recent_move_ring_from_button:(UIButton *)button
{
    if (button == nil) {
        return;
    }
    [[button viewWithTag:FCRecentMoveRingTag] removeFromSuperview];
}

- (void)draw_recent_move_ring_on_button:(UIButton *)button
{
    if (button == nil) {
        return;
    }

    [self remove_recent_move_ring_from_button:button];
    CGFloat cell = MIN(CGRectGetWidth(button.bounds), CGRectGetHeight(button.bounds));
    CGFloat diameter = MAX(10.0, cell * 0.93);
    CGFloat originX = (CGRectGetWidth(button.bounds) - diameter) * 0.5;
    CGFloat originY = (CGRectGetHeight(button.bounds) - diameter) * 0.5;
    UIView *ring = [[UIView alloc] initWithFrame:CGRectMake(originX, originY, diameter, diameter)];
    ring.tag = FCRecentMoveRingTag;
    ring.userInteractionEnabled = NO;
    ring.backgroundColor = UIColor.clearColor;
    ring.layer.cornerRadius = diameter * 0.5;
    ring.layer.borderWidth = MAX(1.2, MIN(2.4, cell * 0.07));
    ring.layer.borderColor = [UIColor colorWithRed:0.12 green:0.43 blue:0.95 alpha:0.95].CGColor;
    ring.layer.shadowColor = UIColor.blackColor.CGColor;
    ring.layer.shadowOpacity = 0.18;
    ring.layer.shadowRadius = 1.2;
    ring.layer.shadowOffset = CGSizeMake(0.0, 0.5);
    [button addSubview:ring];
}

- (void)draw_stone_on_button:(UIButton *)button value:(NSInteger)value
{
    if (button == nil || value == 0) {
        return;
    }

    BOOL isBlack = value > 0;
    NSInteger moveNumber = value > 0 ? value : -value;

    [self remove_preview_dot_from_button:button];
    [self remove_recent_move_ring_from_button:button];
    [self remove_stone_from_button:button];
    [button setImage:nil forState:UIControlStateNormal];
    button.imageView.alpha = 0.0;
    [button setTitle:@"" forState:UIControlStateNormal];
    button.backgroundColor = UIColor.clearColor;

    CGFloat cell = MIN(CGRectGetWidth(button.bounds), CGRectGetHeight(button.bounds));
    CGFloat diameter = MAX(8.0, MIN(CGRectGetWidth(button.bounds), cell * 0.80));
    CGFloat originX = (CGRectGetWidth(button.bounds) - diameter) * 0.5;
    CGFloat originY = (CGRectGetHeight(button.bounds) - diameter) * 0.5;
    UIView *stone = [[UIView alloc] initWithFrame:CGRectMake(originX, originY, diameter, diameter)];
    stone.tag = FCStoneViewTag;
    stone.userInteractionEnabled = NO;
    stone.layer.cornerRadius = diameter * 0.5;
    stone.layer.borderWidth = MAX(0.8, cell * 0.025);
    stone.layer.borderColor = isBlack
        ? [UIColor colorWithWhite:1.0 alpha:0.14].CGColor
        : [UIColor colorWithWhite:0.10 alpha:0.30].CGColor;
    stone.layer.shadowColor = UIColor.blackColor.CGColor;
    stone.layer.shadowOpacity = isBlack ? 0.34 : 0.22;
    stone.layer.shadowRadius = MAX(1.0, cell * 0.08);
    stone.layer.shadowOffset = CGSizeMake(0.0, MAX(0.7, cell * 0.05));
    stone.backgroundColor = isBlack
        ? [UIColor colorWithWhite:0.035 alpha:1.0]
        : [UIColor colorWithWhite:0.98 alpha:1.0];
    // The eye/teaching overlay passes the move number in `value`. Show the
    // complete sequence, including the first move, with an inverted color so
    // the order remains readable on either stone color.
    if (self.drawing_teacher_overlay && moveNumber > 0) {
        UILabel *numberLabel = [[UILabel alloc] initWithFrame:stone.bounds];
        numberLabel.text = [NSString stringWithFormat:@"%ld", (long)moveNumber];
        numberLabel.textAlignment = NSTextAlignmentCenter;
        numberLabel.font = [UIFont systemFontOfSize:MAX(8.0, diameter * 0.40) weight:UIFontWeightSemibold];
        numberLabel.textColor = isBlack
            ? UIColor.whiteColor
            : UIColor.blackColor;
        numberLabel.adjustsFontSizeToFitWidth = YES;
        numberLabel.minimumScaleFactor = 0.72;
        numberLabel.userInteractionEnabled = NO;
        [stone addSubview:numberLabel];
    }
    [button addSubview:stone];
}

- (void)set_empty_cell_at_x:(NSInteger)x y:(NSInteger)y show_preview:(BOOL)showPreview
{
    if (x < 0 || x >= 15 || y < 0 || y >= 15) {
        return;
    }
    UIButton *button = chess_map[x][y];
    if (button == nil) {
        return;
    }

    [self remove_preview_dot_from_button:button];
    [self remove_recent_move_ring_from_button:button];
    [self remove_stone_from_button:button];
    [button setImage:nil forState:UIControlStateNormal];
    button.imageView.alpha = 0.0;
    button.backgroundColor = UIColor.clearColor;

    if (showPreview) {
        CGFloat cell = MIN(CGRectGetWidth(button.bounds), CGRectGetHeight(button.bounds));
        CGFloat side = MAX(12.0, MIN(18.0, cell * 0.65));
        UIView *reticle = [[UIView alloc] initWithFrame:CGRectMake((CGRectGetWidth(button.bounds) - side) * 0.5,
                                                                    (CGRectGetHeight(button.bounds) - side) * 0.5,
                                                                    side,
                                                                    side)];
        reticle.tag = FCPreviewDotTag;
        reticle.userInteractionEnabled = NO;

        UIColor *reticleColor = player_chess_id == 1
            ? [UIColor colorWithWhite:0.05 alpha:0.78]
            : [UIColor colorWithWhite:1.0 alpha:0.90];
        CAShapeLayer *reticleLayer = [CAShapeLayer layer];
        UIBezierPath *reticlePath = [UIBezierPath bezierPath];
        CGPoint center = CGPointMake(side * 0.5, side * 0.5);
        CGFloat diagonal = (CGFloat)(M_SQRT1_2);
        CGFloat outerRadius = side * 0.46;
        CGFloat innerRadius = side * 0.18;
        const CGFloat directions[4][2] = {
            {-diagonal, -diagonal}, {diagonal, -diagonal},
            {-diagonal, diagonal}, {diagonal, diagonal}
        };
        for (NSInteger index = 0; index < 4; index++) {
            CGPoint outer = CGPointMake(center.x + directions[index][0] * outerRadius,
                                        center.y + directions[index][1] * outerRadius);
            CGPoint inner = CGPointMake(center.x + directions[index][0] * innerRadius,
                                        center.y + directions[index][1] * innerRadius);
            [reticlePath moveToPoint:outer];
            [reticlePath addLineToPoint:inner];
        }
        reticleLayer.path = reticlePath.CGPath;
        reticleLayer.fillColor = UIColor.clearColor.CGColor;
        reticleLayer.strokeColor = reticleColor.CGColor;
        reticleLayer.lineWidth = MAX(1.2, cell * 0.055);
        reticleLayer.lineCap = kCALineCapRound;
        reticleLayer.contentsScale = UIScreen.mainScreen.scale;
        reticleLayer.shadowColor = UIColor.blackColor.CGColor;
        reticleLayer.shadowOpacity = 0.20;
        reticleLayer.shadowRadius = 1.2;
        reticleLayer.shadowOffset = CGSizeMake(0.0, 0.5);
        [reticle.layer addSublayer:reticleLayer];
        [button addSubview:reticle];
    }
}

- (void)show_analysis_loading
{
    if (think_flag != 1) {
        return;
    }
    if (![NSThread isMainThread]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [self show_analysis_loading];
        });
        return;
    }
    [self layout_analysis_loading_indicator];
    [self update_analysis_loading_appearance];
    self.analysis_loading_panel.alpha = 1.0;
    self.analysis_loading_panel.hidden = NO;
    self.analysis_status_label.hidden = NO;
    self.analysis_status_label.alpha = 1.0;
    self.analysis_loading_spinner.hidesWhenStopped = NO;
    self.analysis_loading_spinner.hidden = NO;
    [self.analysis_loading_spinner startAnimating];
    [self.view addSubview:self.analysis_loading_panel];
    [self.view bringSubviewToFront:self.analysis_loading_panel];
}

- (void)setup_analysis_loading_indicator
{
    if (_rotater == nil || self.analysis_loading_panel != nil) {
        return;
    }

    // The storyboard outlet is weak. Retain the indicator before detaching it
    // from the storyboard hierarchy, otherwise it can be released mid-move.
    self.analysis_loading_spinner = _rotater;
    UIActivityIndicatorView *spinner = self.analysis_loading_spinner;

    // The storyboard indicator is constrained to the old toolbar position.
    // Move it into a small status panel so it can follow the emoji information
    // bar without changing the storyboard's other layout constraints.
    for (NSLayoutConstraint *constraint in [self.view.constraints copy]) {
        if (constraint.firstItem == spinner || constraint.secondItem == spinner) {
            [self.view removeConstraint:constraint];
        }
    }
    [spinner removeFromSuperview];
    spinner.translatesAutoresizingMaskIntoConstraints = YES;
    spinner.hidesWhenStopped = NO;
    spinner.transform = CGAffineTransformMakeScale(1.35, 1.35);
    spinner.hidden = YES;

    UIView *panel = [[UIView alloc] initWithFrame:CGRectZero];
    panel.userInteractionEnabled = NO;
    panel.layer.cornerRadius = 17.0;
    panel.layer.shadowColor = UIColor.blackColor.CGColor;
    panel.layer.shadowOpacity = 0.16;
    panel.layer.shadowRadius = 4.0;
    panel.layer.shadowOffset = CGSizeMake(0.0, 1.0);
    self.analysis_loading_panel = panel;

    UILabel *statusLabel = [[UILabel alloc] initWithFrame:CGRectZero];
    statusLabel.text = @"思考中...";
    statusLabel.font = [UIFont systemFontOfSize:14.0 weight:UIFontWeightMedium];
    statusLabel.textAlignment = NSTextAlignmentLeft;
    statusLabel.adjustsFontSizeToFitWidth = YES;
    statusLabel.minimumScaleFactor = 0.80;
    statusLabel.userInteractionEnabled = NO;
    self.analysis_status_label = statusLabel;

    [panel addSubview:spinner];
    [panel addSubview:statusLabel];
    [self.view addSubview:panel];
    [self hide_analysis_loading];
}

- (void)layout_analysis_loading_indicator
{
    if (self.analysis_loading_panel == nil || _tech_texter == nil) {
        return;
    }

    CGRect infoFrame = [_tech_texter.superview convertRect:_tech_texter.frame toView:self.view];
    CGFloat panelWidth = MIN(124.0, MAX(104.0, CGRectGetWidth(self.view.bounds) - 16.0));
    CGFloat panelHeight = 34.0;
    CGFloat panelX = CGRectGetWidth(self.view.bounds) - panelWidth - 8.0;
    CGFloat panelY = CGRectGetMidY(infoFrame) - panelHeight * 0.5;
    panelY = MAX(4.0, MIN(panelY, CGRectGetHeight(self.view.bounds) - panelHeight - 4.0));
    self.analysis_loading_panel.frame = CGRectMake(panelX, panelY, panelWidth, panelHeight);
    self.analysis_loading_spinner.frame = CGRectMake(8.0, 3.0, 28.0, 28.0);
    self.analysis_status_label.frame = CGRectMake(44.0,
                                                  0.0,
                                                  panelWidth - 48.0,
                                                  panelHeight);
}

- (void)update_analysis_loading_appearance
{
    if (self.analysis_loading_panel == nil) {
        return;
    }
    self.analysis_loading_panel.backgroundColor = is_darkmode
        ? [UIColor colorWithWhite:0.10 alpha:0.92]
        : [UIColor colorWithWhite:1.0 alpha:0.92];
    self.analysis_status_label.textColor = is_darkmode
        ? [UIColor colorWithWhite:1.0 alpha:0.94]
        : [UIColor colorWithWhite:0.08 alpha:0.90];
}

- (void)hide_analysis_loading
{
    [self.analysis_loading_spinner stopAnimating];
    self.analysis_loading_spinner.hidden = YES;
    self.analysis_status_label.hidden = YES;
    self.analysis_loading_panel.hidden = YES;
    self.analysis_loading_panel.alpha = 0.0;
}

- (void)apply_pending_game_settings
{
    if (self.pending_player_chess_id == 1 || self.pending_player_chess_id == -1) {
        player_chess_id = self.pending_player_chess_id;
        _player_choice_seg.selectedSegmentIndex = (player_chess_id == 1 ? 0 : 1);
        self.pending_player_chess_id = 0;
    }
    if (self.pending_ban_mode >= 0) {
        [ice_fiver set_banmode:self.pending_ban_mode];
        self.pending_ban_mode = -1;
    }
    if (self.pending_difficulty >= 0) {
        player_prefer_difficulty = self.pending_difficulty;
        _difficulty_choice_seg.selectedSegmentIndex = fc_segment_for_difficulty(player_prefer_difficulty);
        self.pending_difficulty = -1;
    }
}

- (void)finish_analysis_and_apply_pending_restart
{
    [self think_finish];
    [self apply_pending_game_settings];
    if (self.restart_pending) {
        self.restart_pending = NO;
        [self perform_restart_now];
    }
}

- (void)perform_restart_now
{
    if (![NSThread isMainThread]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [self perform_restart_now];
        });
        return;
    }

    [self think_finish];
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++) {
            map_state[i][j] = 0;
        }
    }
    [ice_fiver clear_all_data];

    if (player_chess_id == -1) {
        [ice_fiver add_a_chess:7 pl_y:7 mode:1];
        map_state[7][7] = 1;
    }
    focus_x = 0;
    focus_y = 0;
    focus_has_been_selected = false;
    position_changed = false;
    teacher_on = 0;
    self.teacher_overlay_data = nil;
    [self flush_chess_map_according_to:map_state];
}

-(void) initial
{
    Screensize=self.view.bounds;
    if (CGRectGetWidth(Screensize) <= 1.0 || CGRectGetHeight(Screensize) <= 1.0) {
        Screensize=[UIScreen mainScreen].bounds;
    }
    ScreenWidth=(int)Screensize.size.width;
    ScreenHeight=(int)Screensize.size.height;
    int smal=0;
    if(ScreenWidth>ScreenHeight)
        smal=ScreenHeight;
    else smal=ScreenWidth;
    
    NSString* lange = [self usr_lang];
    current_lang=2; // 英语
    if([lange characterAtIndex: 0]=='z')
        current_lang=1; // 汉语
    
    if(current_lang==2){
        NSArray* DA = @[@"Forbidden off ⭕️",@"Forbidden on 🚫"];
        for(int i=0;i<2;i++)
            [_banbar setTitle:DA[i] forSegmentAtIndex:i];
        NSArray *DDC = @[@"Black ⚫️",@"White ⚪️"];
        for(int i=0;i<2;i++)
            [_player_choice_seg setTitle:DDC[i] forSegmentAtIndex:i];
    }
    else{
        NSArray* DA = @[@"禁手关闭 ⭕️",@"禁手开启 🚫"];
        for(int i=0;i<2;i++)
            [_banbar setTitle:DA[i] forSegmentAtIndex:i];
        NSArray *DDC = @[@"黑棋 ⚫️",@"白棋 ⚪️"];
        for(int i=0;i<2;i++)
            [_player_choice_seg setTitle:DDC[i] forSegmentAtIndex:i];
    }
    
    teacher_on=0;
    self.teacher_overlay_data = nil;
    think_flag=0;
    self.restart_pending = NO;
    self.pending_player_chess_id = 0;
    self.pending_ban_mode = -1;
    self.pending_difficulty = -1;
    perwidth=(smal)/19;
    perheight=perwidth;
    focus_y=focus_x=0;
    focus_has_been_selected = false;
    player_chess_id=1;
    if(ice_fiver==nil)
    {
        ice_fiver=[[doublethree alloc] init];
         file_controller=[[filer alloc]init];
        window_controller=[[HDsmallwindow alloc]init_with_fatherwindow:self];
    }
    player_prefer_difficulty=0;
    _difficulty_choice_seg.selectedSegmentIndex=fc_segment_for_difficulty(player_prefer_difficulty);
}
-(NSString*) usr_lang
{
    NSString * language = [[NSLocale preferredLanguages] objectAtIndex:0];
    return language;
}
- (void)viewDidLoad {
    [super viewDidLoad];
    [self initial];
    [self setup_analysis_loading_indicator];
    self.analysis_queue = [[NSOperationQueue alloc] init];
    self.analysis_queue.name = @"ice-five-chess.ai-analysis";
    self.analysis_queue.maxConcurrentOperationCount = 1;
    self.analysis_queue.qualityOfService = NSQualityOfServiceUserInitiated;
    [self setup_board_surface];
    
    for(int i=0;i<15;i++)
        for(int j=0;j<15;j++)
        {
            [self set_chess_map_btn_with_x:i y:j];
            map_state[i][j]=0;
        }
    [self read_all_from_file:@"autosave"];
    [self think_finish];
    srand(time(0));
    int rander=rand()%3;
    if(rander==0)
        _tech_texter.text=@"😃😅😮🤔🤭🤗(我应该赢了)😲😧😦😢😰😨😱(又被坑了。。)";
    else if(rander==1){
        _tech_texter.text=@"Designed by Tropical fish 🐠🐠";
    }
    else{
        _tech_texter.text=@"又来下棋 ?? 😂😂";
    }
    UIApplication *app=[UIApplication sharedApplication];
    [[NSNotificationCenter defaultCenter]addObserver:self selector:@selector(applicationWillResignActive:) name:UIApplicationWillResignActiveNotification object:app];
    // Do any additional setup after loading the view.
}

- (void)viewDidLayoutSubviews
{
    [super viewDidLayoutSubviews];
    CGRect bounds = self.view.bounds;
    if (CGRectGetWidth(bounds) > 1.0 && CGRectGetHeight(bounds) > 1.0) {
        Screensize = bounds;
        ScreenWidth = (int)CGRectGetWidth(bounds);
        ScreenHeight = (int)CGRectGetHeight(bounds);
        CGFloat shortestSide = MIN(CGRectGetWidth(bounds), CGRectGetHeight(bounds));
        perwidth = shortestSide / 19.0;
        perheight = perwidth;
    }
    [self layout_board];
    [self layout_analysis_loading_indicator];
}

-(void) applicationWillResignActive:(NSNotification*)notification
{
    [self save_all_to_file:@"autosave"];
}
-(void) viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    self.navigationController.navigationBarHidden=true;
    [self update_darkmode];
    // [self update_current_lang];
}
-(void) update_darkmode
{
    if (@available(iOS 13.0, *)) {
        UIColor *dynamicColor = [UIColor colorWithDynamicProvider:^UIColor * _Nonnull(UITraitCollection * _Nonnull traitCollection) {
            switch (traitCollection.userInterfaceStyle) {
                case UIUserInterfaceStyleLight: {/// 亮色模式颜色
                    self->is_darkmode=false;
                    return [UIColor whiteColor];
                }
                    break;
                case UIUserInterfaceStyleDark: {/// 暗色模式颜色
                    self->is_darkmode=true;
                    return [UIColor colorNamed:@"darkbkcolor"];
                }
                    break;
                default: {
                    self->is_darkmode=false;
                    return [UIColor whiteColor];
                }
                    break;
            }
        }];
        [_sudoback setBackgroundColor:dynamicColor];
       // [self.view addSubview:_sudoback];
    } else {
        // Fallback on earlier versions
        [_sudoback setBackgroundColor:UIColor.whiteColor];
       // [self.view addSubview:_sudoback];
    }
    [self update_board_surface_appearance];
    [self update_analysis_loading_appearance];

}
-(void) save_all_to_file:(NSString*)filename
{
    NSString* pack_cont=[file_controller pack_chessboard:map_state];
    NSString* main_title=[[NSString alloc]initWithFormat:@"%@_player_main",filename];
    NSString* player_ch_title=[[NSString alloc]initWithFormat:@"%@_player_ch",filename];
    NSString* player_df_title=[[NSString alloc]initWithFormat:@"%@_player_df",filename];
    NSString* player_ban_title=[[NSString alloc]initWithFormat:@"%@_player_ban",filename];
    NSString* player_stack_title=[[NSString alloc]initWithFormat:@"%@_player_stack",filename];
    
    int ban_choice=(int)_ban_choice.selectedSegmentIndex;
    NSString* pl_ch=[[NSString alloc]initWithFormat:@"%d",player_chess_id];
    NSString* pl_df=[[NSString alloc]initWithFormat:@"%d",player_prefer_difficulty];
    NSString* pl_ban=[[NSString alloc]initWithFormat:@"%d",ban_choice];
    
    [file_controller File_Save:pack_cont to:main_title];
    [file_controller File_Save:pl_ch to:player_ch_title];
    [file_controller File_Save:pl_df to:player_df_title];
    [file_controller File_Save:pl_ban to:player_ban_title];
    
    
    int stacker[225][2];
    int stack_height;
    stack_height=[ice_fiver export_stack:stacker];

    NSString* stack_contents=[file_controller pack_chess_stack:stacker height:stack_height];
    // NSLog(@"height %d : %@",stack_height,stack_contents);
    [file_controller File_Save:stack_contents to:player_stack_title];
    
}
-(int) read_all_from_file:(NSString*)filename
{
    NSString* main_title=[[NSString alloc]initWithFormat:@"%@_player_main",filename];
    NSString* str=[file_controller File_read:main_title];
    if(str.length<220)
        return 0;
    
    [file_controller release_chessboard:map_state data:str];
    [ice_fiver import_from_board:map_state];
    
    NSString* player_ch_title=[[NSString alloc]initWithFormat:@"%@_player_ch",filename];
    NSString* player_df_title=[[NSString alloc]initWithFormat:@"%@_player_df",filename];
    NSString* player_ban_title=[[NSString alloc]initWithFormat:@"%@_player_ban",filename];
    NSString* player_stack_title=[[NSString alloc]initWithFormat:@"%@_player_stack",filename];
    NSString* pl_ch=[file_controller File_read:player_ch_title];
    NSString* pl_df=[file_controller File_read:player_df_title];
    NSString* pl_ban=[file_controller File_read:player_ban_title];
    player_chess_id=(int)pl_ch.integerValue;
    if(player_chess_id==-1)
    {
        _player_choice_seg.selectedSegmentIndex=1;
    }
    else{
        _player_choice_seg.selectedSegmentIndex=0;
    }
    player_prefer_difficulty=(int)pl_df.integerValue;
    if(player_prefer_difficulty<0||player_prefer_difficulty>4)
        player_prefer_difficulty=0;
    _difficulty_choice_seg.selectedSegmentIndex=fc_segment_for_difficulty(player_prefer_difficulty);
    
    if(_ban_choice.selectedSegmentIndex!=pl_ban.integerValue)
    {
        _ban_choice.selectedSegmentIndex=pl_ban.integerValue;
        [ice_fiver set_banmode:(int)pl_ban.integerValue];
    }
    NSString* stack_contents=[file_controller File_read:player_stack_title];
    int stacker[225][2];
    int stack_height;
    stack_height=[file_controller release_chess_stack:stacker data:stack_contents];

    [ice_fiver import_stack:stacker height:stack_height];
    [self flush_chess_map_according_to:map_state];
    return 1;
}
-(void) set_chess_map_btn_with_x:(int)x y:(int)y
{
    [self set_base_block:0 x:x y:y back_color:UIColor.clearColor title_color:UIColor.lightGrayColor];
}
-(void) try_next_step_for_x:(int)move_x
                           y:(int)move_y
                      player:(int)move_player
                  difficulty:(int)difficulty
                   requestID:(NSUInteger)requestID
{
    if(think_flag==1 && move_x >= 0 && move_x < 15 && move_y >= 0 && move_y < 15)
    {
        if(move_player==1)
        {
            int res=[ice_fiver add_a_chess:move_x pl_y:move_y mode:1];
            
            if(res!=1)
            {
                [self analysis_next_step_for_player:move_player difficulty:difficulty];
                [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                    if (self->think_flag != 1 || self.analysis_request_id != requestID)
                        return;
                    self->_tech_texter.text=[self->ice_fiver get_now_tech];
                    [self flush_chess_map_according_to:self->map_state];
                    if([self->ice_fiver win_state]==-1)
                    {
                        self->_tech_texter.text=@" 🎉🎉";
                        if (!self.restart_pending)
                            [self White_win_restart_funct];
                    }
                    [self finish_analysis_and_apply_pending_restart];
                }];
            }
            else{
                [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                    if (self->think_flag != 1 || self.analysis_request_id != requestID)
                        return;
                    self->_tech_texter.text=[self->ice_fiver get_now_tech];
                    [self flush_chess_map_according_to:self->map_state];
                    if([self->ice_fiver win_state]==1)
                    {
                        self->_tech_texter.text=@" 🌝🌚";
                        if (!self.restart_pending)
                            [self Black_win_restart_funct];
                    }
                    [self finish_analysis_and_apply_pending_restart];
                }];
            }
        }
        else{
            int res=[ice_fiver add_a_chess:move_x pl_y:move_y mode:-1];
            if(res==-1)
            {
                [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                    if (self->think_flag != 1 || self.analysis_request_id != requestID)
                        return;
                    self->_tech_texter.text=[self->ice_fiver get_now_tech];
                    [self flush_chess_map_according_to:self->map_state];
                    if([self->ice_fiver win_state]==-1)
                    {
                        self->_tech_texter.text=@" 🌝🌚";
                        if (!self.restart_pending)
                            [self White_win_restart_funct];
                    }
                    [self finish_analysis_and_apply_pending_restart];
                }];
            }
            else{
                [self analysis_next_step_for_player:move_player difficulty:difficulty];
                [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                    if (self->think_flag != 1 || self.analysis_request_id != requestID)
                        return;
                    self->_tech_texter.text=[self->ice_fiver get_now_tech];
                    [self flush_chess_map_according_to:self->map_state];
                    if([self->ice_fiver win_state]==1)
                    {
                        self->_tech_texter.text=@" 🎉🎉";
                        if (!self.restart_pending)
                            [self Black_win_restart_funct];
                    }
                    [self finish_analysis_and_apply_pending_restart];
                }];
            }
        }
    }
}
-(void)think_start
{
    if (![NSThread isMainThread]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [self think_start];
        });
        return;
    }
    think_flag=1;
    NSUInteger requestID = ++self.analysis_request_id;
    [self hide_analysis_loading];
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(NSEC_PER_SEC)),
                   dispatch_get_main_queue(), ^{
        if (self->think_flag == 1 && self.analysis_request_id == requestID) {
            [self show_analysis_loading];
        }
    });
}
-(void)think_finish
{
    if (![NSThread isMainThread]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [self think_finish];
        });
        return;
    }
    self.analysis_request_id += 1;
    think_flag=0;
    focus_has_been_selected=false;
    [self hide_analysis_loading];
}
-(void) paint_chess_map_with_x:(long)x y:(long)y val:(int)val
{
    if (x < 0 || x >= 15 || y < 0 || y >= 15 || chess_map[x][y] == nil) {
        return;
    }
    [self remove_preview_dot_from_button:chess_map[x][y]];
    if(val==1 || val==-1){
        [self draw_stone_on_button:chess_map[x][y] value:val];
    }
    else if(val==0){
        [self set_empty_cell_at_x:x y:y show_preview:NO];
    }
    
}
-(void) paint_focus_chess_map_with_x:(long)x y:(long)y val:(int)val
{
    if (x < 0 || x >= 15 || y < 0 || y >= 15 || val == 0) {
        return;
    }
    [self draw_recent_move_ring_on_button:chess_map[x][y]];
}
-(void) clear_chess_map_with_x:(int)x y:(int)y
{
    if (teacher_on == 1) {
        // Leaving a board cell for the eye button must not remove the
        // numbered teaching overlay.
        return;
    }
    [self set_empty_cell_at_x:x y:y show_preview:NO];
}
-(void) flush_chess_map_according_to:(int[15][15])row_map
{
    for(int i=0;i<15;i++)
    {
        for(int j=0;j<15;j++)
        {
            [self paint_chess_map_with_x:i y:j val:row_map[i][j]];
        }
    }
    // Re-apply the eye-mode sequence after any board refresh. Toolbar/button
    // interactions can trigger a refresh, but must not erase the numbered
    // teaching stones that are currently being shown.
    [self render_teacher_overlay];
    BOOL has_chess = NO;
    for (int i = 0; i < 15 && !has_chess; i++) {
        for (int j = 0; j < 15; j++) {
            if (row_map[i][j] != 0) {
                has_chess = YES;
                break;
            }
        }
    }
    if(ice_fiver!=nil && has_chess)
    {
        int pos[2] = {-1, -1};
        int lcolor=[ice_fiver get_last_pos_return_color:pos];
        if(lcolor != 0 && pos[0] >= 0 && pos[0] < 15 && pos[1] >= 0 && pos[1] < 15 && row_map[pos[0]][pos[1]]==lcolor)
            [self paint_focus_chess_map_with_x:pos[0] y:pos[1] val:lcolor];
    }
}

- (void)render_teacher_overlay
{
    if (teacher_on != 1 || self.teacher_overlay_data.length != sizeof(int) * 15 * 15) {
        return;
    }

    int mapper[15][15] = {};
    [self.teacher_overlay_data getBytes:mapper length:sizeof(mapper)];
    self.drawing_teacher_overlay = YES;
    for (int i = 0; i < 15; i++) {
        for (int j = 0; j < 15; j++) {
            if (mapper[i][j] != 0) {
                [self draw_stone_on_button:chess_map[i][j] value:mapper[i][j]];
            }
        }
    }
    self.drawing_teacher_overlay = NO;
}
-(void) copy_state_map:(int[15][15])aimed_map from:(int[15][15])map_ext
{
    for(int i=0;i<15;i++)
        for(int j=0;j<15;j++)
            aimed_map[i][j]=map_ext[i][j];
    
}
-(void) set_base_block:(int)val x:(int)x y:(int)y back_color:(UIColor*)background_color title_color:(UIColor*)title_color
{
    CGRect gridFrame = [self board_grid_frame];
    CGRect position=CGRectMake(0, 0, 0, 0);
    if (chess_map[x][y] == nil) {
        position=CGRectMake(CGRectGetMinX(gridFrame) + x * self.board_cell_size,
                            CGRectGetMinY(gridFrame) + y * self.board_cell_size,
                            self.board_cell_size,
                            self.board_cell_size);
    }
    
    chess_map[x][y]=[self AddBlockBtn:chess_map[x][y] frame:position action:@selector(focus_click:) val:val blockrow:x blockcol:y Backgroundcolor:background_color TitleColor:title_color];
    
    [self set_empty_cell_at_x:x y:y show_preview:NO];
}
-(UIButton*) AddBlockBtn:(UIButton*)btn frame:(CGRect)frame
                  action:(SEL)action val:(int)val blockrow:(int)row blockcol:(int)col Backgroundcolor:(UIColor*)backcolor TitleColor:(UIColor*)titlecolor
{
    if(btn==nil)
    {
        btn=[[UIButton alloc] init];
        btn.tag=row*15+col;
        [btn setShowsTouchWhenHighlighted:true];
        [btn addTarget:self action:action forControlEvents:UIControlEventTouchUpInside];
        btn.frame = frame;
        btn.imageView.contentMode = UIViewContentModeScaleAspectFit;
        btn.imageView.clipsToBounds = NO;
        btn.adjustsImageWhenHighlighted = NO;
        if(btn.tag>100)
            btn.titleLabel.font=[UIFont systemFontOfSize:23 weight:UIFontWeightRegular];
        else
            btn.titleLabel.font=[UIFont systemFontOfSize:23 weight:UIFontWeightLight];
    }
    if(val!=0)
    {
        NSString* td=[[NSString alloc] initWithFormat:@"%d",val];
        [btn setTitle:td forState:UIControlStateNormal];
    }
    else{
        [btn setTitle:@"" forState:UIControlStateNormal];
    }
    [btn setBackgroundColor:UIColor.clearColor];
    [btn setTitleColor:titlecolor forState:UIControlStateNormal];

    //监听btn
    [self.view addSubview:btn];
    return btn;
    
}

- (IBAction)clear_btn_act:(UIBarButtonItem *)sender {
    [self restart_funct];
}
-(void)restart_funct
{
    if (![NSThread isMainThread]) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [self restart_funct];
        });
        return;
    }
    // Never clear the AI engine while its analysis thread is using it.
    if (think_flag == 1) {
        self.restart_pending = YES;
        return;
    }
    [self apply_pending_game_settings];
    [self perform_restart_now];
}
-(void) White_win_restart_funct
{
    NSArray* button_names=@[@"⚔️ 重新开始",@"🕳 更换棋子"];
    NSString* tit = @"白棋 ⚪️ 胜利";
    if(current_lang==2)
    {
        button_names=@[@"⚔️ Play again",@"🕳 Change your chess"];
        tit = @"White win";
    }
    [window_controller Rich_NewsMessage_with_two_button:@"⚪️ 🌟" message:@"" button_nameset:button_names funct1:@selector(restart_funct) funct2:@selector(change_chess_and_restart)];
}
-(void) Black_win_restart_funct
{
    NSArray* button_names=@[@"⚔️ 重新开始",@"🕳 更换棋子"];
    NSString* tit = @"黑棋 ⚫️ 胜利";
    if(current_lang==2)
    {
        button_names=@[@"⚔️ Play again",@"🕳 Change your chess"];
        tit = @"Black win";
    }
    [window_controller Rich_NewsMessage_with_two_button:@"⚫️ 🌟" message:@"" button_nameset:button_names funct1:@selector(restart_funct) funct2:@selector(change_chess_and_restart)];
}
-(void) analysis_next_step_for_player:(int)player difficulty:(int)difficulty
{
        [ice_fiver import_from_board:map_state];
        switch (difficulty) {
            case 0:
                [ice_fiver harsh_analysisboard:-player];
                break;
            case 1:
                [ice_fiver easy_analysisboard:-player];
                break;
            case 2:
                [ice_fiver egg_analysisboard:-player];
                break;
            case 3:
                [ice_fiver four_star_analysisboard:-player];
                break;
            case 4:
                [ice_fiver five_star_analysisboard:-player];
                break;
            default:
                [ice_fiver harsh_analysisboard:-player];
                break;
        }
        [ice_fiver export_current_board:map_state];
}

- (void)enqueue_analysis_for_x:(int)x
                              y:(int)y
                         player:(int)player
                     difficulty:(int)difficulty
                      requestID:(NSUInteger)requestID
{
    if (self.analysis_queue == nil) {
        self.analysis_queue = [[NSOperationQueue alloc] init];
        self.analysis_queue.name = @"ice-five-chess.ai-analysis";
        self.analysis_queue.maxConcurrentOperationCount = 1;
        self.analysis_queue.qualityOfService = NSQualityOfServiceUserInitiated;
    }
    [self.analysis_queue addOperationWithBlock:^{
        dispatch_semaphore_t finished = dispatch_semaphore_create(0);
        NSThread *analysisThread = [[NSThread alloc] initWithBlock:^{
            @autoreleasepool {
                [self try_next_step_for_x:x
                                       y:y
                                  player:player
                              difficulty:difficulty
                               requestID:requestID];
            }
            dispatch_semaphore_signal(finished);
        }];
        // The proof engine has a large stack frame and can recurse deeply.
        // NSOperationQueue worker stacks are too small for the five-star path.
        analysisThread.stackSize = 4 * 1024 * 1024;
        // Keep the dedicated large-stack thread at the same QoS as the
        // operation that waits for it, avoiding a priority inversion warning.
        analysisThread.qualityOfService = NSQualityOfServiceUserInitiated;
        [analysisThread start];
        dispatch_semaphore_wait(finished, DISPATCH_TIME_FOREVER);
    }];
}

-(IBAction) focus_click:(UIButton *)sender
{
    NSInteger tager=[sender tag];
    if (tager < 0 || tager >= 225) {
        return;
    }
    if (teacher_on == 1) {
        // The eye mode is read-only. The first board tap dismisses the hint
        // and is intentionally not treated as a move confirmation.
        teacher_on = 0;
        self.teacher_overlay_data = nil;
        [self flush_chess_map_according_to:map_state];
        return;
    }
    NSInteger target_x = tager / 15;
    NSInteger target_y = tager % 15;
    
    if([ice_fiver win_state]!=0)
        return;
    if(think_flag==1)
        return;
    
    if(!focus_has_been_selected || focus_x != target_x || focus_y != target_y)
    {
        position_changed=true;
        if(focus_has_been_selected && focus_x >= 0 && focus_x < 15 &&
           focus_y >= 0 && focus_y < 15 && map_state[focus_x][focus_y]==0)
            [self clear_chess_map_with_x:(int)focus_x y:(int)focus_y];

        focus_x=target_x;
        focus_y=target_y;
        focus_has_been_selected=true;
        [self set_op_focus_sign];
        return;
    }
    else
    {
        position_changed=false;
        if(map_state[focus_x][focus_y]==0)
        {
            if(player_chess_id==1)
            {
                if([ice_fiver current_banmode]==1)
                {
                    if([ice_fiver banned_point:focus_x j:focus_y]==1)
                    {
                        [window_controller Simple_alertMessage_With_Title:@"🚫" andMessage:@"⚠️⚠️"];
                        return;
                    }
                }

                map_state[focus_x][focus_y]=1;
                focus_has_been_selected=false;
                [self paint_chess_map_with_x:focus_x y:focus_y val:1];
                [self think_start];
                //[[NSNotificationCenter defaultCenter] postNotificationName:@"trynow" object:nil];
                [self enqueue_analysis_for_x:(int)focus_x
                                           y:(int)focus_y
                                      player:player_chess_id
                                  difficulty:player_prefer_difficulty
                                   requestID:self.analysis_request_id];
               
            }
            else{
                map_state[focus_x][focus_y]=-1;
                focus_has_been_selected=false;
                [self paint_chess_map_with_x:focus_x y:focus_y val:-1];
                [self think_start];
                //[[NSNotificationCenter defaultCenter] postNotificationName:@"trynow" object:nil];
                [self enqueue_analysis_for_x:(int)focus_x
                                           y:(int)focus_y
                                      player:player_chess_id
                                  difficulty:player_prefer_difficulty
                                   requestID:self.analysis_request_id];
            }
        }
        return;
    }
}
-(void)set_op_focus_sign
{
    if(map_state[focus_x][focus_y]==0)
    {
        [self set_empty_cell_at_x:focus_x y:focus_y show_preview:YES];
    }
    else{
        
        int elt=0;
        bool dis_empty[4]={};
        if(focus_y>0 && map_state[focus_x][focus_y-1]==0)
        {
            dis_empty[0]=true;
        }
        else if(focus_y<14 && map_state[focus_x][focus_y+1]==0){
            dis_empty[1]=true;
        }
        else if(focus_x>0 &&map_state[focus_x-1][focus_y]==0){
            dis_empty[2]=true;
        }
        else if(focus_x<14 &&map_state[focus_x+1][focus_y]==0){
            dis_empty[3]=true;
        }
        
        for(int i=0;i<4;i++)
        {
            if(dis_empty[i]==true)
                elt=1;
        }
        if(elt==1)
        {
            elt=rand()%4;
            while (dis_empty[elt]==false) {
                elt=rand()%4;
            }
            switch (elt) {
                case 0:
                    focus_y-=1;
                    [self set_empty_cell_at_x:focus_x y:focus_y show_preview:YES];
                    break;
                case 1:
                    focus_y+=1;
                    [self set_empty_cell_at_x:focus_x y:focus_y show_preview:YES];
                    break;
                case 2:
                    focus_x-=1;
                    [self set_empty_cell_at_x:focus_x y:focus_y show_preview:YES];
                    break;
                case 3:
                    focus_x+=1;
                    [self set_empty_cell_at_x:focus_x y:focus_y show_preview:YES];
                    break;
                default:
                    break;
            }
        }
    }
}
- (IBAction)ban_choice_change:(UISegmentedControl *)sender {
    if (think_flag == 1) {
        self.pending_ban_mode = (int)sender.selectedSegmentIndex;
        self.restart_pending = YES;
        return;
    }
    [ice_fiver set_banmode:(int)sender.selectedSegmentIndex];
    [self restart_funct];
}
- (IBAction)helping_predict:(UIBarButtonItem *)sender {
    if (think_flag == 1) {
        return;
    }
    teacher_on^=1;
    if (teacher_on == 1) {
        int mapper[15][15] = {};
        [ice_fiver teaching_current_step:mapper];
        self.teacher_overlay_data = [NSData dataWithBytes:mapper length:sizeof(mapper)];
    } else {
        self.teacher_overlay_data = nil;
    }
    [self flush_chess_map_according_to:map_state];

}
- (IBAction)player_chess_change:(UISegmentedControl *)sender {
    int requested_chess_id = (sender.selectedSegmentIndex == 0 ? 1 : -1);
    if (think_flag == 1) {
        self.pending_player_chess_id = requested_chess_id;
        self.restart_pending = YES;
        return;
    }
    player_chess_id = requested_chess_id;
    [self restart_funct];
}
- (IBAction)diff_change:(UISegmentedControl *)sender {
    int requested_difficulty = fc_difficulty_for_segment(sender.selectedSegmentIndex);
    if (think_flag == 1) {
        self.pending_difficulty = requested_difficulty;
        return;
    }
    player_prefer_difficulty = requested_difficulty;
}
-(void)change_chess_and_restart
{
    int requested_chess_id = (player_chess_id == 1 ? -1 : 1);
    if (think_flag == 1) {
        self.pending_player_chess_id = requested_chess_id;
        self.restart_pending = YES;
        return;
    }
    player_chess_id = requested_chess_id;
    _player_choice_seg.selectedSegmentIndex = (player_chess_id == 1 ? 0 : 1);
    [self restart_funct];
}
- (IBAction)undo_act:(UIBarButtonItem *)sender {
    if (think_flag == 1) {
        return;
    }
    [ice_fiver withdraw_two_steps];
    [ice_fiver export_current_board:map_state];
    _tech_texter.text=@"😠😡😤";
    [self flush_chess_map_according_to:map_state];
}
@end

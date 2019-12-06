//
//  doublethree.h
//  ice five chess
//
//  Created by 王子诚 on 2019/5/11.
//  Copyright © 2019 王子诚. All rights reserved.
//

#ifndef doublethree_h
#define doublethree_h

#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>


@interface doublethree : NSObject
{
    int modelA[2][15];
    int modelB[2][15];
    int modelorigin[2][15];
    int banned_mode;
    int A_win_time;
    int B_win_time;
    int total_train_time;
    int r,g,b;
    int moder_now;
    int train_on;
    int chessid;
    int vsmode;
    int game_end;
    int maxid;
    int border_x_min,border_x_max;
    int border_y_min,border_y_max;
    int chessboard[15][15];
    bool enemy_force_map[15][15];
    int process[225][2];
    int totalprocess;
    int updater;
    int maxchessline;
    int border;
    int last_pos[2];
    int last_color;
    NSString* now_tech;
}
-(doublethree*)init;
-(int)get_last_pos_return_color:(int[2])pos;
//string string_of_id(int i,int j);
-(void) export_current_board:(int[15][15])board;
-(void) import_from_board:(int[15][15])board;
-(void) set_banmode:(int)mode;
-(int) current_banmode;
-(int) win_state;
-(int) Abs:(int)x;
-(int) checkidea :(int) x y :(int) y mode:(int) mode scale:(int) scale border:(int) border;
-(int) no_way_defense_conditon:(int) mode;
-(int) keypoint:(int) x y:(int) y mode:(int) mode type:(int) type;
-(int) keypoint_save:(int) x y:(int) y mode:(int) mode aimer:(int[2]) aimer ;
-(int) doublethreetest:(int) x y:(int) y mode:(int) mode type:(int) type border: (int) border;
-(int) mustdevelopment:(int) x y:(int) y mode:(int) mode border:(int) border;
-(int) doublethreetest_save:(int) x y:(int) y mode:(int) mode savers:(int[7]) saver border:(int) border;
-(void) egg_analysisboard:(int) mode;
-(void) harsh_analysisboard:(int) mode;
-(void) easy_analysisboard:(int) mode;
-(int) add_a_chess:(int) pl_x pl_y:(int) pl_y mode:(int) mode;
-(void)emoji_techer:(double)sc;
-(int) dead_point:(int[100][2])saver mode:(int)mode;
-(int) need_point:(int[100][2])saver mode:(int) mode;
-(int) advance_four_hide_attack:(int) x y:(int) y mode:(int) mode tower:(int) tower;
-(int) advance_doublethree_hide_attack:(int) x y:(int) y mode:(int) mode tower:(int) tower;
-(int) advance_super_fast_attack:(int) x y:(int) y mode:(int) mode tower:(int) tower;
-(int) fight_back_level:(int) x y:(int) y mode:(int) mode;
-(int) fast_attack_point:(int[100][2])attackers mode:(int) mode;
-(int) force_point:(int[100][2])saver mode:(int) mode;
-(int) exist_force:(int) mode;
-(int) exist_sudden_fight_back:(int) x y:(int) y mode:(int) mode;

-(int) double_three_point:(int[100][2])saver mode:(int) mode;
-(int) point_score:(int)i y:(int)j mode:(int)mode;
-(int) imagine:(int) x y:(int) y mode:(int) mode tower:(int) tower;
-(int) export_stack:(int[225][2])stacker;
-(void) import_stack:(int[225][2])stacker height:(int)stack_height;
-(int) killpoint:(int) x y:(int) y mode:(int) mode;
-(int) banned_point:(int) i j:(int) j;
-(void) teaching_current_step:(int[15][15])paint_map;
//string randcolor();
-(void) withdraw_two_steps;
-(void) update_border;
-(void) clear_all_data;
-(NSString*)get_now_tech;

@end

#endif /* doublethree_h */

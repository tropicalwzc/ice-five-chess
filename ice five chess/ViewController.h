//
//  ViewController.h
//  ice five chess
//
//  Created by 王子诚 on 2019/5/11.
//  Copyright © 2019 王子诚. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "doublethree.h"
#import "smallwindow.h"
#import "file.h"
#ifndef ViewController_h
#define ViewController_h
@interface ViewController : UIViewController
{
    int map_state[15][15];
    UIButton* chess_map[15][15];
    doublethree* ice_fiver;
    filer* file_controller;
    smallwindow* window_controller;
    NSThread* threader[2];
    int thread_num;
    long focus_x,focus_y;
    bool position_changed;
    int player_prefer_difficulty;
    int think_flag;
    bool is_darkmode;
    long current_lang;
    @protected
    CGRect Screensize;
    int ScreenWidth;
    int ScreenHeight;
    int perwidth;
    int perheight;
    int player_chess_id;
    int teacher_on;
}
-(void)restart_funct;
-(void)change_chess_and_restart;

@end
#endif

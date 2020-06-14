//
//  ViewController.m
//  ice five chess
//
//  Created by 王子诚 on 2019/5/11.
//  Copyright © 2019 王子诚. All rights reserved.
//

#import "ViewController.h"

@interface ViewController ()
@property (weak, nonatomic) IBOutlet UISegmentedControl *player_choice_seg;
@property (weak, nonatomic) IBOutlet UIButton *player_sure_btn;
@property (weak, nonatomic) IBOutlet UISegmentedControl *difficulty_choice_seg;
@property (weak, nonatomic) IBOutlet UISegmentedControl *banbar;
@property (weak, nonatomic) IBOutlet UILabel *db_texter;
@property (weak, nonatomic) IBOutlet UISegmentedControl *ban_choice;
@property (weak, nonatomic) IBOutlet UILabel *sudoback;
@property (weak, nonatomic) IBOutlet UIActivityIndicatorView* rotater;
@end

@implementation ViewController
-(void) initial
{
    Screensize=[UIScreen mainScreen].bounds;
    ScreenWidth=(int)Screensize.size.width;
    ScreenHeight=(int)Screensize.size.height;
    int smal=0;
    if(ScreenWidth>ScreenHeight)
        smal=ScreenHeight;
    else smal=ScreenWidth;
    
    perwidth=(smal)/15;
    perheight=perwidth;
    focus_y=focus_x=0;
    player_chess_id=1;
    think_flag=0;
    teacher_on=0;
    ice_fiver=[[doublethree alloc] init];
    file_controller=[[filer alloc]init];
    window_controller=[[smallwindow alloc]init_with_fatherwindow:self];
    NSString* lange = [self usr_lang];
    current_lang=2; // 英语
    if([lange characterAtIndex: 0]=='z')
        current_lang=1; // 汉语
    
    if(current_lang==2){
        NSArray* DA = @[@"Forbidden off ⭕️",@"Forbidden on 🚫"];
        for(int i=0;i<2;i++)
            [_banbar setTitle:DA[i] forSegmentAtIndex:i];
        
        DA=@[@"Black ⚫️",@"White ⚪️"];
        for(int i=0;i<2;i++)
            [_player_choice_seg setTitle:DA[i] forSegmentAtIndex:i];
    }
    else{
        NSArray* DA = @[@"禁手关闭 ⭕️",@"禁手开启 🚫"];
        for(int i=0;i<2;i++)
            [_banbar setTitle:DA[i] forSegmentAtIndex:i];
        DA=@[@"黑棋 ⚫️",@"白棋 ⚪️"];
        for(int i=0;i<2;i++)
            [_player_choice_seg setTitle:DA[i] forSegmentAtIndex:i];
    }

    
    _player_sure_btn.tag=1001;
}
- (void)viewDidLoad {
    [super viewDidLoad];
    [self initial];
    
    for(int i=0;i<15;i++)
        for(int j=0;j<15;j++)
        {
            [self set_chess_map_btn_with_x:i y:j];
            map_state[i][j]=0;
        }
    [self read_all_from_file:@"autosave"];
    srand(time(0));
    int rander=rand()%3;
    [self think_finish];
    if(current_lang==2){
        if(rander==0)
            _db_texter.text=@"😃😅😮🤔🤭🤗(我应该赢了)\n😲😧😦😢😰😨😱(又被坑了)";
        else if(rander==1){
            _db_texter.text=@"🦆🦆🏔 Tropical fish 🐠🐠";
        }
        else{
            _db_texter.text=@"又来下棋 ?? 😂😂";
        }
    }
    else{
        if(rander==0)
        _db_texter.text=@"😃😅😮🤔🤭🤗(I am going to win)\n😲😧😦😢😰😨😱(Oh! shit)";
        else if(rander==1){
            _db_texter.text=@"🦆🦆⛰ Tropical fish 🐠🐠";
        }
        else{
            _db_texter.text=@"Again ?? 😂😂";
        }
    }

    thread_num=2;
    for(int p=0;p<thread_num;p++)
    {
        threader[p] = [[NSThread alloc] initWithTarget:self selector:@selector(createRunloopByNormal) object:nil] ;
        [threader[p] start];
    }
    UIApplication *app=[UIApplication sharedApplication];
    [[NSNotificationCenter defaultCenter]addObserver:self selector:@selector(applicationWillResignActive:) name:UIApplicationWillResignActiveNotification object:app];
    // Do any additional setup after loading the view.
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
    
}
-(NSString*) usr_lang
{
    NSString * language = [[NSLocale preferredLanguages] objectAtIndex:0];
    return language;
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
    _difficulty_choice_seg.selectedSegmentIndex=player_prefer_difficulty;
    
    if(_ban_choice.selectedSegmentIndex!=pl_ban.integerValue)
    {
        _ban_choice.selectedSegmentIndex=pl_ban.integerValue;
        [ice_fiver set_banmode:(int)pl_ban.integerValue];
    }
    NSString* stack_contents=[file_controller File_read:player_stack_title];
    int stacker[225][2];
    int stack_height;
    stack_height=[file_controller release_chess_stack:stacker data:stack_contents];
    // if(player_chess_id==-1)
    // {
    [ice_fiver import_stack:stacker height:stack_height];
    // }
    // else [ice_whiter import_stack:stacker height:stack_height];
    
    [self flush_chess_map_according_to:map_state];
    return 1;
}
-(void) set_chess_map_btn_with_x:(int)x y:(int)y
{
    [self set_base_block:0 x:x y:y back_color:UIColor.whiteColor title_color:UIColor.lightGrayColor];
}
-(void) paint_chess_map_with_x:(long)x y:(long)y val:(int)val
{
    if(val==1){
        [chess_map[x][y] setImage:[UIImage imageNamed:@"black_chess_pic"] forState:UIControlStateNormal];
    }
    if(val==-1){
        [chess_map[x][y] setImage:[UIImage imageNamed:@"white_chess_pic"] forState:UIControlStateNormal];
    }
    if(val==0){
        [chess_map[x][y] setImage:[UIImage imageNamed:@"aimed_pic"] forState:UIControlStateNormal];
    }
    [self.view addSubview:chess_map[x][y]];
}
-(void) clear_chess_map_with_x:(int)x y:(int)y
{
    [chess_map[x][y] setImage:[UIImage imageNamed:@"aimed_pic"] forState:UIControlStateNormal];
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
    if(ice_fiver!=nil)
    {
        int pos[2];
        int lcolor;
        lcolor=[ice_fiver get_last_pos_return_color:pos];

        
        if(row_map[pos[0]][pos[1]]==lcolor)
            [self paint_focus_chess_map_with_x:pos[0] y:pos[1] val:lcolor];
    }
}
-(void) paint_focus_chess_map_with_x:(long)x y:(long)y val:(int)val
{
    if(x>0&&map_state[x-1][y]==0)
        [chess_map[x-1][y] setImage:[UIImage imageNamed:@"d_righter"] forState:UIControlStateNormal];
    else if(x<14&&map_state[x+1][y]==0)
        [chess_map[x+1][y] setImage:[UIImage imageNamed:@"d_lefter"] forState:UIControlStateNormal];
    else if(y<14&&map_state[x][y+1]==0)
        [chess_map[x][y+1] setImage:[UIImage imageNamed:@"d_upper"] forState:UIControlStateNormal];
    else if(y>0&&map_state[x][y-1]==0)
        [chess_map[x][y-1] setImage:[UIImage imageNamed:@"d_downer"] forState:UIControlStateNormal];
    else{
        if(val==1){
            [chess_map[x][y] setImage:[UIImage imageNamed:@"black_chess_pic_l"] forState:UIControlStateNormal];
        }
        if(val==-1){
            [chess_map[x][y] setImage:[UIImage imageNamed:@"white_chess_pic_l"] forState:UIControlStateNormal];
        }
    }
}
-(void) copy_state_map:(int[15][15])aimed_map from:(int[15][15])map_ext
{
    for(int i=0;i<15;i++)
        for(int j=0;j<15;j++)
            aimed_map[i][j]=map_ext[i][j];
    
}
-(void) set_base_block:(int)val x:(int)x y:(int)y back_color:(UIColor*)background_color title_color:(UIColor*)title_color
{
    
    CGRect position=CGRectMake(0, 0, 0, 0);
    if(chess_map[x][y]==nil)
        position=CGRectMake(x*perwidth+(ScreenWidth-15*perwidth)/2, ScreenHeight/2+(y-8)*perheight, perwidth,perheight);
    
    chess_map[x][y]=[self AddBlockBtn:chess_map[x][y] frame:position action:@selector(focus_click:) val:val blockrow:x blockcol:y Backgroundcolor:background_color TitleColor:title_color];
    
    [chess_map[x][y] setImage:[UIImage imageNamed:@"aimed_pic"] forState:UIControlStateNormal];
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
    [btn setBackgroundColor:backcolor];
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
    for(int i=0;i<15;i++)
        for(int j=0;j<15;j++)
        {
            map_state[i][j]=0;
        }
    [ice_fiver import_from_board:map_state];
    [ice_fiver clear_all_data];

    
    if(player_chess_id==-1)
    {
        [ice_fiver add_a_chess:7 pl_y:7 mode:1];
        map_state[7][7]=1;
    }
    [self flush_chess_map_according_to:map_state];
    [self following_act_with_x:100 y:0];
}
-(void) White_win_restart_funct
{
    NSArray* button_names=@[@"重新开始",@"更换棋子"];
    NSString* tit = @"白棋胜利";
    if(current_lang==2)
    {
        button_names=@[@"⚔️ Play again",@"🕳 Change your chess"];
        tit = @"White ⚪️ win";
    }
    [window_controller Rich_NewsMessage_with_two_button:tit message:@"" button_nameset:button_names funct1:@selector(restart_funct) funct2:@selector(change_chess_and_restart)];
}
-(void) Black_win_restart_funct
{
    NSArray* button_names=@[@"⚔️ 重新开始",@"🕳 更换棋子"];
    NSString* tit = @"黑棋 ⚫️ 胜利";
    if(current_lang==2)
    {
        button_names=@[@"Play again",@"Change your chess"];
        tit = @"Black win";
    }
    [window_controller Rich_NewsMessage_with_two_button:tit message:@"" button_nameset:button_names funct1:@selector(restart_funct) funct2:@selector(change_chess_and_restart)];
}
-(void) try_next_step
{
    if(think_flag==1)
    {
        if(player_chess_id==1)
        {
            int res=[ice_fiver add_a_chess:(int)focus_x pl_y:(int)focus_y mode:1];
            
            if(res!=1)
            {
                [self analysis_next_step];
                [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                    self->_db_texter.text=[self->ice_fiver get_now_tech];
                    [self flush_chess_map_according_to:self->map_state];
                    if([self->ice_fiver win_state]==-1)
                    {
                        if(self->current_lang==1){
                            self->_db_texter.text=@"白棋胜利 🎉🎉";
                        }
                        else{
                            self->_db_texter.text=@"White win 🎉🎉";
                        }

                        [self White_win_restart_funct];
                    }
                    [self think_finish];
                }];
            }
            else{
                [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                    self->_db_texter.text=[self->ice_fiver get_now_tech];
                    [self flush_chess_map_according_to:self->map_state];
                    if([self->ice_fiver win_state]==1)
                    {
                        if(self->current_lang==1){
                            self->_db_texter.text=@"黑棋胜利 😯😯";
                        }
                        else{
                            self->_db_texter.text=@"Black win 😯😯";
                        }

                        [self Black_win_restart_funct];
                    }
                    [self think_finish];
                }];
            }
        }
        else{
            int res=[ice_fiver add_a_chess:(int)focus_x pl_y:(int)focus_y mode:-1];
            if(res==-1)
            {
                [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                    self->_db_texter.text=[self->ice_fiver get_now_tech];
                    [self flush_chess_map_according_to:self->map_state];
                    if([self->ice_fiver win_state]==-1)
                    {
                        if(self->current_lang==1){
                            self->_db_texter.text=@"白棋胜利 😯😯";
                        }
                        else{
                            self->_db_texter.text=@"White win 😯😯";
                        }

                        [self White_win_restart_funct];
                    }
                    [self think_finish];
                }];
            }
            else{
                [self analysis_next_step];
                [[NSOperationQueue mainQueue] addOperationWithBlock:^{
                    self->_db_texter.text=[self->ice_fiver get_now_tech];
                    [self flush_chess_map_according_to:self->map_state];
                    if([self->ice_fiver win_state]==1)
                    {
                        if(current_lang==1){
                            self->_db_texter.text=@"黑棋胜利 🎉🎉";
                        }
                        else{
                            self->_db_texter.text=@"Black win 🎉🎉";
                        }

                        [self Black_win_restart_funct];
                    }
                    [self think_finish];
                }];
            }
        }
    }
}
- (void)createRunloopByNormal{
    @autoreleasepool {
        
        //添加port源，保证runloop正常轮询，不会创建后直接退出。
        [[NSRunLoop currentRunLoop] addPort:[NSPort port] forMode:NSDefaultRunLoopMode];
        
        //开启runloop
        [[NSRunLoop currentRunLoop] run];
        //  NSLog(@"hel %d",rand()%100);
    }
}
-(void)think_start
{
    [_rotater setHidden:false];
    [_rotater startAnimating];
    think_flag=1;
}
-(void)think_finish
{
    [_rotater setHidden:true];
    [_rotater startAnimating];
    think_flag=0;
}
-(void) analysis_next_step
{
        [ice_fiver import_from_board:map_state];
        switch (player_prefer_difficulty) {
            case 0:
                [ice_fiver harsh_analysisboard:-player_chess_id];
                break;
            case 1:
                [ice_fiver easy_analysisboard:-player_chess_id];
                break;
            case 2:
                [ice_fiver egg_analysisboard:-player_chess_id];
                break;
            default:
                [ice_fiver harsh_analysisboard:-player_chess_id];
                break;
        }
        [ice_fiver export_current_board:map_state];
}
-(IBAction) focus_click:(UIButton *)sender
{
    long tager=[sender tag];
    if([ice_fiver win_state]!=0)
        return;
    
    if(focus_x==tager/15&&focus_y==tager%15)
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
                        if(current_lang==1){
                            [window_controller Simple_alertMessage_With_Title:@"🚫" andMessage:@"Forbidden warning⚠️"];
                        }
                        else{
                            [window_controller Simple_alertMessage_With_Title:@"🚫" andMessage:@"Forbidden warning⚠️"];
                        }
                        return;
                    }
                }
                
                map_state[focus_x][focus_y]=1;
                [self paint_chess_map_with_x:focus_x y:focus_y val:1];
                [self think_start];
                //[[NSNotificationCenter defaultCenter] postNotificationName:@"trynow" object:nil];
                [self performSelector:@selector(try_next_step) onThread:threader[0] withObject:nil waitUntilDone:NO];
                
            }
            else{
                [self paint_chess_map_with_x:focus_x y:focus_y val:-1];
                map_state[focus_x][focus_y]=-1;
                [self think_start];
                //[[NSNotificationCenter defaultCenter] postNotificationName:@"trynow" object:nil];
                [self performSelector:@selector(try_next_step) onThread:threader[0] withObject:nil waitUntilDone:NO];
            }
        }
        return;
    }
    else{
        position_changed=true;
        if(map_state[focus_x][focus_y]==0)
            [self clear_chess_map_with_x:focus_x y:focus_y];
        
        focus_x=tager/15;
        focus_y=tager%15;
        [self set_op_focus_sign];
    }
}
-(void)set_op_focus_sign
{
    if(map_state[focus_x][focus_y]==0)
    {
        [chess_map[focus_x][focus_y] setImage:[UIImage imageNamed:@"focus_aimed_pic"]  forState:UIControlStateNormal];
    }
    else{
        
        int elt=0;
        bool dis_empty[4]={};
        if(focus_y>1 && map_state[focus_x][focus_y-1]==0)
        {
            dis_empty[0]=true;
        }
        else if(focus_y<14 && map_state[focus_x][focus_y+1]==0){
            dis_empty[1]=true;
        }
        else if(focus_x>1 &&map_state[focus_x-1][focus_y]==0){
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
                    [chess_map[focus_x][focus_y] setImage:[UIImage imageNamed:@"focus_aimed_pic"]  forState:UIControlStateNormal];
                    break;
                case 1:
                    focus_y+=1;
                    [chess_map[focus_x][focus_y] setImage:[UIImage imageNamed:@"focus_aimed_pic"]  forState:UIControlStateNormal];
                    break;
                case 2:
                    focus_x-=1;
                    [chess_map[focus_x][focus_y] setImage:[UIImage imageNamed:@"focus_aimed_pic"]  forState:UIControlStateNormal];
                    break;
                case 3:
                    focus_x+=1;
                    [chess_map[focus_x][focus_y] setImage:[UIImage imageNamed:@"focus_aimed_pic"]  forState:UIControlStateNormal];
                    break;
                default:
                    break;
            }
        }
    }
}
- (IBAction)helping_predict:(UIBarButtonItem *)sender {
    teacher_on^=1;
    if(teacher_on==1)
    {
        int mapper[15][15]={};
        [ice_fiver teaching_current_step:mapper];
        for(int i=0;i<15;i++)
            for(int j=0;j<15;j++)
            {
                if(mapper[i][j]==0)
                    continue;
                
                if(mapper[i][j]>0)
                {
                    NSString* pic_name=[[NSString alloc]initWithFormat:@"blk%d",mapper[i][j]];
                    [chess_map[i][j] setImage:[UIImage imageNamed:pic_name] forState:UIControlStateNormal];
                }
                else{
                    NSString* pic_name=[[NSString alloc]initWithFormat:@"whi%d",-mapper[i][j]];
                    [chess_map[i][j] setImage:[UIImage imageNamed:pic_name] forState:UIControlStateNormal];
                }
            }
    }
    else{
        [self flush_chess_map_according_to:map_state];
    }
    
}
- (IBAction)ban_choice_change:(UISegmentedControl *)sender {
    [ice_fiver set_banmode:(int)sender.selectedSegmentIndex];
    [self restart_funct];
}
- (IBAction)player_chess_change:(UISegmentedControl *)sender {
    if(sender.selectedSegmentIndex==0)
        player_chess_id=1;
    else{
        player_chess_id=-1;
    }
    [self restart_funct];
}
- (IBAction)diff_change:(UISegmentedControl *)sender {
    player_prefer_difficulty=(int)[sender selectedSegmentIndex];
}
-(void)change_chess_and_restart
{
    if(player_chess_id==1)
    {
        player_chess_id=-1;
        _player_choice_seg.selectedSegmentIndex=1;
    }
    else{
        player_chess_id=1;
        _player_choice_seg.selectedSegmentIndex=0;
    }
    [self restart_funct];
}
- (IBAction)restart_act:(UIBarButtonItem *)sender {
    [self restart_funct];
}
- (IBAction)sure_here:(UIButton *)sender {
    [self focus_click:chess_map[focus_x][focus_y]];
}
- (IBAction)undo_act:(UIBarButtonItem *)sender {

        [ice_fiver withdraw_two_steps];
        [ice_fiver export_current_board:map_state];
    _db_texter.text=@"😠😡😤";
    [self flush_chess_map_according_to:map_state];
}
-(void) following_act_with_x:(int)x y:(int)y
{

    _player_sure_btn.frame=CGRectMake(2000, 1000, 40, 40);
    if(x>90 || map_state[x][y]!=0)
        return;
    
    if(y>1 && map_state[x][y-1]==0)
    {
        [ _player_sure_btn setImage :[UIImage imageNamed:@"d_downer"] forState:UIControlStateNormal];
        float px = x*perwidth-perwidth/3+5;
        float py = ScreenHeight/2+(y-9.1)*perheight+5;
        _player_sure_btn.frame=CGRectMake(px, py, 30, 30);
    }
    else if(y<14 && map_state[x][y+1]==0)
    {
        [ _player_sure_btn setImage :[UIImage imageNamed:@"d_upper"] forState:UIControlStateNormal];
        float px = x*perwidth-perwidth/3+5;
        float py = ScreenHeight/2+(y-6.9)*perheight+5;
        _player_sure_btn.frame=CGRectMake(px, py, 30, 30);
    }
    else if(x<14&& map_state[x+1][y]==0)
    {
        [ _player_sure_btn setImage :[UIImage imageNamed:@"d_lefter"] forState:UIControlStateNormal];
        float px = x*perwidth-perwidth/3+perwidth*1.1+5;
        float py = ScreenHeight/2+(y-8)*perheight+5;
        _player_sure_btn.frame=CGRectMake(px, py, 30, 30);
    }
    else if(x>1&& map_state[x-1][y]==0)
    {
        [ _player_sure_btn setImage :[UIImage imageNamed:@"d_righter"] forState:UIControlStateNormal];
        float px = x*perwidth-perwidth/3-perwidth*1.1+5;
        float py = ScreenHeight/2+(y-8)*perheight+5;
        _player_sure_btn.frame=CGRectMake(px, py, 40, 40);
    }
    
    [self.view addSubview:_player_sure_btn];
}
@end

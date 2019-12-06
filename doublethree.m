//
//  doublethree.m
//  ice five chess
//
//  Created by 王子诚 on 2019/5/11.
//  Copyright © 2019 王子诚. All rights reserved.
//

#import <Foundation/Foundation.h>
#include "doublethree.h"


@implementation doublethree

-(doublethree*)init
{
    
    //1400 ,2000 ,1000 ,8500 ,6000 , 8000, 30000, 28500, 6000
    for(int i=0;i<12;i++)
        switch (i) {
            case 0:
                modelorigin[0][i]=1400;// 必然发展棋
                modelorigin[1][i]=1400;
                break;
            case 1:
                modelorigin[0][i]=2000; // 单三进攻
                modelorigin[1][i]=2000;
                break;
            case 2:
                modelorigin[0][i]=1000;// 单三防御
                modelorigin[1][i]=1000;
                break;
            case 3:
                modelorigin[0][i]=8500; // 双三进攻
                modelorigin[1][i]=8500;
                break;
            case 4:
                modelorigin[0][i]=6000;// 双三防御
                modelorigin[1][i]=6000;
                break;
            case 5:
                modelorigin[0][i]=8000;// 挖陷阱
                modelorigin[1][i]=8000;
                break;
            case 6:
                modelorigin[0][i]=30000;// 形成链式进攻
                modelorigin[1][i]=30000;
                break;
            case 7:
                modelorigin[0][i]=28500;// 想象进攻成立
                modelorigin[1][i]=28500;
                break;
            case 8:
                modelorigin[0][i]=6000; // 预防性防御
                modelorigin[1][i]=6000;
                break;
            case 9:
                modelorigin[0][i]=2000; // 预言家
                modelorigin[1][i]=2000;
                break;
            case 10:
                modelorigin[0][i]=1000; // 预言家防御
                modelorigin[1][i]=1000;
                break;
            case 11:
                modelorigin[0][i]=1200; // 房地产攻击法
                modelorigin[1][i]=1000;
                break;
            default:
                break;
        }
    for(int i=0;i<2;i++)
        for(int j=0;j<15;j++)
            modelA[i][j]=modelorigin[i][j];
    
    A_win_time=0;
    B_win_time=0;
    total_train_time=0;
    moder_now=1;
    train_on=0;
    chessid=0;
    vsmode=1;
    game_end=0;
    maxid=0;
    border_x_min=100;
    border_x_max=-1;
    border_y_min=100;
    border_y_max=-1;
    for(int i=0;i<15;i++)
        for(int j=0;j<15;j++)
            chessboard[i][j]=0;
    for(int i=0;i<225;i++)
        for(int j=0;j<2;j++)
            process[i][j]=0;
    updater=0;
    now_tech=[[NSString alloc]init];
    maxchessline=15;
    border=15;
    banned_mode=0;
    totalprocess=0;
    return self;
}

-(int) exist_three:(int) x y:(int) y mode:(int) mode
{
    int all = 0;
    chessboard[x][y] = mode;
    for (int i = border_x_min - 1; i < border_x_max + 2; i++)
    {
        if (i < 0 || i > maxchessline - 1)
            continue;
        
        for (int j = border_y_min - 1; j < border_y_max + 2; j++)
        {
            if (j > maxchessline - 1 || j < 0)
                continue;
            if (chessboard[i][j] != 0)
                continue;
            if ([self doublethreetest:i y:j mode:-mode type:0 border:15] != 0 || [self keypoint:i y:j mode:-mode type:0] != 0)
            {
                all++;
            }
            if ([self doublethreetest:i y:j mode:mode type:0 border:15] != 0 || [self keypoint:i y:j mode:mode type:0] != 0)
            {
                all--;
            }
        }
    }
    chessboard[x][y] = 0;
    return all;
}
-(int) point_score:(int)i y:(int)j mode:(int)mode
{
    if (chessboard[i][j] != 0)
        return -50000;
    
    if (banned_mode == 1 && mode == 1)
    {
        if( mode==1)
            if ([self banned_point:i j:j] == 1)
                return -50000;
    }
    int attackscore = rand()%1000;
    int defensescore = 0;
    
    if (totalprocess <= 5)
    {
        if ([self bad_start:i j:j chessid:totalprocess] == 1)
        {
            attackscore -= 10000;
        }
    }
    else{
        attackscore = rand()%2000;
    }
    
    int read_val_id = mode == 1 ? 1 : 0;
    
    int must_num = [self mustdevelopment:i y:j mode:mode border:15];
    attackscore += modelA[read_val_id][0] * must_num;
    defensescore +=  [self exist_three:i y:j mode:mode] * (-300);
    //defensescore +=  [self exist_three:i y:j mode:-mode] * (800);
    
    int threepoint_white = [self doublethreetest:i y:j mode:-1 type:0 border:15];
    int threepoint_black = [self doublethreetest:i y:j mode:1 type:0 border:15];
    int keywhite = [self keypoint:i y:j mode:-1 type:0];
    int keyblack = [self keypoint:i y:j mode:1 type:0];
    
    if (mode == -1)
    {
        attackscore += modelA[read_val_id][11]*keywhite;
        attackscore += modelA[read_val_id][1] * (threepoint_white);
        //defensescore += modelA[read_val_id][2] * (threepoint_black);
    }
    else
    {
        attackscore +=modelA[read_val_id][11]*keyblack;
        // attackscore += modelA[read_val_id][2] * (threepoint_white);
        defensescore += modelA[read_val_id][1] * (threepoint_black);
    }
    
    
    if (threepoint_white > 1 && mode == -1 || threepoint_black > 1 && mode == 1)
        attackscore += modelA[read_val_id][3];
    if (threepoint_white > 1 && mode == 1 || threepoint_black > 1 && mode == -1)
        defensescore += modelA[read_val_id][4];
    
    int A_score = defensescore + attackscore;
    
    chessboard[i][j] = mode;
    int us_force_exist = [self exist_force:mode];
    if (us_force_exist == 1)
    {
        A_score += modelA[read_val_id][5];
    }
    

    /*
     if ([self no_way_defense_conditon:-mode] == 1)
     A_score += modelA[read_val_id][6];
     
     
     if([self exist_no_use_environment:mode]!=0 )
     {
     A_score-=20000;
     }
     
    else if(((mode==-1&&keywhite==0&&threepoint_white==0)||(mode==1&&keyblack==0&&threepoint_black==0))&&us_force_exist==0&&[self exist_imagine:mode]!=0)
    {
        A_score-=20000;
    }
    */
    
    chessboard[i][j] = 0;
    
    if (mode == 1 && (threepoint_black > 0 || keyblack > 0) || mode == -1 && (threepoint_white > 0 || keywhite > 0))//smart attack
    {
        chessboard[i][j] = mode;
        if ([self no_way_defense_conditon:-mode] == 1)
            A_score += modelA[read_val_id][6];
        
        chessboard[i][j] = 0;
        
        if ([self imagine:i y:j mode:mode tower:0] == 1)
        {
            A_score += modelA[read_val_id][7];
        }
    }
    
    if (mode == -1 && (threepoint_black > 0 || keyblack > 0) || mode == 1 && (threepoint_white > 0 || keywhite > 0))//smart defense
    {
        if ([self imagine:i y:j mode:-mode tower:0] == 1)
        {
            A_score += modelA[read_val_id][8];//Random.Range(5000, 7000)
        }
    }
    if(mode==1)
    {
        chessboard[i][j]=mode;
        int ag = [self future_attack_possiblity:i y:j mode:mode];
        A_score+=ag*modelA[read_val_id][9];
        chessboard[i][j]=0;
    }
    
    
    return A_score ;
}
-(int) exist_no_use_environment:(int)mode
{
    for(int i=border_x_min;i<border_x_max;i++)
        for(int j=border_y_min;j<border_y_max;j++)
        {
            if(chessboard[i][j]!=0)
                continue;
            if(enemy_force_map[i][j])
            {
                chessboard[i][j]=-mode;
                if([self no_way_defense_conditon:mode]!=0)
                {
                    chessboard[i][j]=0;
                    return 1;
                }
                chessboard[i][j]=0;
            }
        }
    return 0;
}
-(int) easy_point_score:(int)i y:(int)j mode:(int)mode
{
    if (chessboard[i][j] != 0)
        return -50000;
    if (banned_mode == 1 && mode == 1)
    {
        if(mode==1)
            if ([self banned_point:i j:j] == 1)
                return -50000;
    }
    int attackscore = 0;
    int defensescore = 0;
    
    if (totalprocess <= 5)
    {
        if ([self bad_start:i j:j chessid:totalprocess] == 1)
        {
            attackscore -= 10000;
        }
    }
    
    int read_val_id = mode == 1 ? 1 : 0;
    
    int must_num = [self mustdevelopment:i y:j mode:mode border:15];
    attackscore += modelA[read_val_id][0] * must_num;
    defensescore +=  [self exist_three:i y:j mode:mode] * (-100);
    
    int threepoint_white = [self doublethreetest:i y:j mode:-1 type:0 border:15];
    int threepoint_black = [self doublethreetest:i y:j mode:1 type:0 border:15];
    
    if (mode == -1)
    {
        attackscore += modelA[read_val_id][1] * threepoint_white;
        defensescore += modelA[read_val_id][2] * threepoint_black;
    }
    else
    {
        attackscore += modelA[read_val_id][2] * threepoint_white;
        defensescore += modelA[read_val_id][1] * threepoint_black;
    }
    
    
    if (threepoint_white > 1 && mode == -1 || threepoint_black > 1 && mode == 1)
        attackscore += modelA[read_val_id][3];
    if (threepoint_white > 1 && mode == 1 || threepoint_black > 1 && mode == -1)
        defensescore += modelA[read_val_id][4];
    
    
    int keywhite = [self keypoint:i y:j mode:-1 type:0];
    int keyblack = [self keypoint:i y:j mode:1 type:0];
    int A_score = defensescore + attackscore;
    
    chessboard[i][j] = mode;
    int us_force_exist = [self exist_force:mode];
    if (us_force_exist == 1)
    {
        A_score += modelA[read_val_id][5];
    }
    chessboard[i][j] = 0;
    
    if (mode == 1 && (threepoint_black > 0 || keyblack > 0) || mode == -1 && (threepoint_white > 0 || keywhite > 0))//smart attack
    {
        chessboard[i][j] = mode;
        if ([self no_way_defense_conditon:-mode] == 1)
            A_score += modelA[read_val_id][6];
        
        chessboard[i][j] = 0;
        
        if ([self imagine:i y:j mode:mode tower:0] == 1)
        {
            A_score += modelA[read_val_id][7];
        }
        
    }
    
    if (mode == -1 && (threepoint_black > 0 || keyblack > 0) || mode == 1 && (threepoint_white > 0 || keywhite > 0))//smart defense
    {
        if ([self imagine:i y:j mode:-mode tower:2] == 1)
        {
            A_score += modelA[read_val_id][8];//Random.Range(5000, 7000)
        }
    }
    
    return A_score - rand()%1000;
}

-(int) egg_point_score:(int)i y:(int)j mode:(int)mode
{
    if (chessboard[i][j] != 0)
        return -50000;
    if (banned_mode == 1 && mode == 1)
    {
        if(mode==1)
            if ([self banned_point:i j:j] == 1)
                return -50000;
    }
    int attackscore = 0;
    int defensescore = 0;
    
    if (totalprocess <= 5)
    {
        if ([self bad_start:i j:j chessid:totalprocess] == 1)
        {
            attackscore -= 10000;
        }
    }
    
    int read_val_id = mode == 1 ? 1 : 0;
    
    int must_num = [self mustdevelopment:i y:j mode:mode border:15];
    attackscore += modelA[read_val_id][0] * must_num;
    defensescore +=  [self exist_three:i y:j mode:mode] * (-1400);
    
    int threepoint_white = [self doublethreetest:i y:j mode:-1 type:0 border:15];
    int threepoint_black = [self doublethreetest:i y:j mode:1 type:0 border:15];
    
    if (mode == -1)
    {
        attackscore += modelA[read_val_id][1] * threepoint_white;
        defensescore += modelA[read_val_id][2] * threepoint_black;
    }
    else
    {
        attackscore += modelA[read_val_id][2] * threepoint_white;
        defensescore += modelA[read_val_id][1] * threepoint_black;
    }
    
    
    if (threepoint_white > 1 && mode == -1 || threepoint_black > 1 && mode == 1)
        attackscore += modelA[read_val_id][3];
    if (threepoint_white > 1 && mode == 1 || threepoint_black > 1 && mode == -1)
        defensescore += modelA[read_val_id][4];
    
    int A_score = defensescore + attackscore;
    return A_score - rand()%1000;
}

-(int) no_way_defense_conditon:(int) mode;
{
    int need_defense[100][2] ={};
    int enemy_force[100][2] = {};
    int enemy_force_total = [self force_point:enemy_force mode:-mode];
    int need_defense_total = [self need_point:need_defense mode:mode];
    if (enemy_force_total > 0 && need_defense_total == 0)
        return 1;
    
    return 0;
}
-(int) checkidea :(int) x y :(int) y mode:(int) mode scale:(int) scale border:(int) border;
{
    if(x<0||x>border-1||y<0||y>border-1)
        return 0;
    if(chessboard[x][y]!=0)
        return 0;
    
    int accumulate=0;
    int bis=0;
    
    if(scale!=5)
        bis=1;
    if(scale==2||scale==3)
        bis=2;
    
    chessboard[x][y]=mode;
    int res=0;
    int aiming=mode*scale;
    
    for(int i=x+1-scale-bis;i<x+1;i++)
    {
        if(i+scale+bis-1<0||i+scale+bis-1>border-1||i<0||i>border-1)
        {
            continue;
        }
        res=0;
        for(int j=i;j<i+scale+bis&&j>=0&&j<border;j++)
        {
            res+=chessboard[j][y];
        }
        
        if(res==aiming)
        {
            chessboard[x][y]=0;
            return 1;
        }
        accumulate++;
    }
    for(int i=y+1-scale-bis;i<y+1;i++)
    {
        if(i+scale+bis-1<0||i+scale+bis-1>border-1||i<0||i>border-1)
        {
            continue;
        }
        res=0;
        for(int j=i;j<i+scale+bis&&j>=0&&j<border;j++)
        {
            res+=chessboard[x][j];
        }
        
        if(res==aiming)
        {
            chessboard[x][y]=0;
            return 1;
        }
        accumulate++;
    }
    
    for(int i=1-scale-bis;i<1;i++)
    {
        if(x+i<0||x+i+scale+bis-1>border-1||y+i<0||y+i+scale+bis-1>border-1)
        {
            continue;
        }
        res=0;
        for(int j=i;j<i+scale+bis;j++)
        {
            res+=chessboard[x+j][y+j];
        }
        if(res==aiming)
        {
            chessboard[x][y]=0;
            return 1;
        }
        accumulate++;
    }
    
    
    for(int i=1-scale-bis;i<1;i++)
    {
        if(x-i>border-1||x-i-scale-bis+1<0||y+i<0||y+i+scale+bis-1>border-1)
        {
            continue;
        }
        res=0;
        for(int j=i;j<i+scale+bis;j++)
        {
            res+=chessboard[x-j][y+j];
        }
        if(res==aiming)
        {
            chessboard[x][y]=0;
            return 1;
        }
        accumulate++;
    }
    
    chessboard[x][y]=0;
    return 0;
}

-(int) keypoint:(int) x y:(int) y mode:(int) mode type:(int) type;
{
    if(x<0||x>maxchessline-1||y<0||y>maxchessline-1)
        return 0;
    if(chessboard[x][y]!=0)
        return 0;
    
    chessboard[x][y]=mode;
    int blanker[2]={};
    int oldblanker[2]={};
    int oldexist=0;
    int tid=0;
    int typer=0;
    for(int d=x-5;d<x+1;d++)
    {
        int counter=0;
        for(int j=d;j<d+5&&counter<2;j++)
        {
            if(j<0||j>maxchessline-1)
            {
                counter=0;
                break;
            }
            if(chessboard[j][y]==-mode)
            {
                counter=0;
                break;
            }
            if(chessboard[j][y]==0)
            {
                blanker[0]=j;
                blanker[1]=y;
                counter++;
            }
            
        }
        
        if(counter==1)
        {
            if(oldexist==0||blanker[0]!=oldblanker[0]||blanker[1]!=oldblanker[1])
                tid++;
            
            if(oldexist==0)
            {
                oldblanker[0]=blanker[0];
                oldblanker[1]=blanker[1];
                oldexist=1;
            }
            typer=1;
        }
    }
    for(int d=y-5;d<y+1;d++)
    {
        int counter=0;
        for(int j=d;j<d+5&&counter<2;j++)
        {
            if(j<0||j>maxchessline-1)
            {
                counter=0;
                break;
            }
            if(chessboard[x][j]==-mode)
            {
                counter=0;
                break;
            }
            if(chessboard[x][j]==0)
            {
                counter++;
                blanker[0]=x;
                blanker[1]=j;
            }
            
        }
        
        if(counter==1)
        {
            if(oldexist==0||blanker[0]!=oldblanker[0]||blanker[1]!=oldblanker[1])
                tid++;
            
            if(oldexist==0)
            {
                oldblanker[0]=blanker[0];
                oldblanker[1]=blanker[1];
                oldexist=1;
            }
            typer=2;
        }
    }
    
    for(int d=-5;d<1;d++)
    {
        int counter=0;
        for(int j=d;j<d+5&&counter<2;j++)
        {
            if(x+j<0||x+j>maxchessline-1||y+j<0||y+j>maxchessline-1)
            {
                counter=0;
                break;
            }
            if(chessboard[x+j][y+j]==-mode)
            {
                counter=0;
                break;
            }
            if(chessboard[x+j][y+j]==0)
            {
                counter++;
                blanker[0]=x+j;
                blanker[1]=y+j;
            }
            
        }
        
        if(counter==1)
        {
            if(oldexist==0||blanker[0]!=oldblanker[0]||blanker[1]!=oldblanker[1])
                tid++;
            
            if(oldexist==0)
            {
                oldblanker[0]=blanker[0];
                oldblanker[1]=blanker[1];
                oldexist=1;
            }
            typer=3;
        }
    }
    for(int d=-5;d<1;d++)
    {
        int counter=0;
        for(int j=d;j<d+5&&counter<2;j++)
        {
            if(x-j<0||x-j>maxchessline-1||y+j<0||y+j>maxchessline-1)
            {
                counter=0;
                break;
            }
            if(chessboard[x-j][y+j]==-mode)
            {
                counter=0;
                break;
            }
            if(chessboard[x-j][y+j]==0)
            {
                counter++;
                blanker[0]=x-j;
                blanker[1]=y+j;
            }
            
        }
        
        if(counter==1)
        {
            if(oldexist==0||blanker[0]!=oldblanker[0]||blanker[1]!=oldblanker[1])
                tid++;
            
            if(oldexist==0)
            {
                oldblanker[0]=blanker[0];
                oldblanker[1]=blanker[1];
                oldexist=1;
            }
            typer=4;
        }
    }
    
    chessboard[x][y]=0;
    if(type==1)
        return typer*10+tid;
    return tid;
    
}

-(int) keypoint_save:(int) x y:(int) y mode:(int) mode aimer:(int[2]) oldblanker ;
{
    if(x<0||x>maxchessline-1||y<0||y>maxchessline-1)
        return 0;
    if(chessboard[x][y]!=0)
        return 0;
    if(mode!=1&&mode!=-1)
        return 0;
    
    chessboard[x][y]=mode;
    int blanker[2]={};
    
    for(int d=x-5;d<x+1;d++)
    {
        int counter=0;
        for(int j=d;j<d+5&&counter<2;j++)
        {
            if(j<0||j>maxchessline-1)
            {
                counter=0;
                break;
            }
            if(chessboard[j][y]==-mode)
            {
                counter=0;
                break;
            }
            if(chessboard[j][y]==0)
            {
                blanker[0]=j;
                blanker[1]=y;
                counter++;
            }
        }
        
        if(counter==1)
        {
            oldblanker[0]=blanker[0];
            oldblanker[1]=blanker[1];
            chessboard[x][y]=0;
            return 1;
        }
    }
    for(int d=y-5;d<y+1;d++)
    {
        int counter=0;
        for(int j=d;j<d+5&&counter<2;j++)
        {
            if(j<0||j>maxchessline-1)
            {
                counter=0;
                break;
            }
            if(chessboard[x][j]==-mode)
            {
                counter=0;
                break;
            }
            if(chessboard[x][j]==0)
            {
                counter++;
                blanker[0]=x;
                blanker[1]=j;
            }
            
        }
        
        if(counter==1)
        {
            oldblanker[0]=blanker[0];
            oldblanker[1]=blanker[1];
            chessboard[x][y]=0;
            return 1;
        }
    }
    
    for(int d=-5;d<1;d++)
    {
        int counter=0;
        for(int j=d;j<d+5&&counter<2;j++)
        {
            if(x+j<0||x+j>maxchessline-1||y+j<0||y+j>maxchessline-1)
            {
                counter=0;
                break;
            }
            if(chessboard[x+j][y+j]==-mode)
            {
                counter=0;
                break;
            }
            if(chessboard[x+j][y+j]==0)
            {
                counter++;
                blanker[0]=x+j;
                blanker[1]=y+j;
            }
            
        }
        
        if(counter==1)
        {
            oldblanker[0]=blanker[0];
            oldblanker[1]=blanker[1];
            chessboard[x][y]=0;
            return 1;
        }
    }
    for(int d=-5;d<1;d++)
    {
        int counter=0;
        for(int j=d;j<d+5&&counter<2;j++)
        {
            if(x-j<0||x-j>maxchessline-1||y+j<0||y+j>maxchessline-1)
            {
                counter=0;
                break;
            }
            if(chessboard[x-j][y+j]==-mode)
            {
                counter=0;
                break;
            }
            if(chessboard[x-j][y+j]==0)
            {
                counter++;
                blanker[0]=x-j;
                blanker[1]=y+j;
            }
        }
        
        if(counter==1)
        {
            oldblanker[0]=blanker[0];
            oldblanker[1]=blanker[1];
            chessboard[x][y]=0;
            return 1;
        }
    }
    
    chessboard[x][y]=0;
    return 0;
    
}
-(int) mustdevelopment:(int) x y:(int) y mode:(int) mode border:(int) border;
{
    if (x < 0 || x >= border || y < 0 || y >= border)
        return 0;
    if (chessboard[x][y] != 0)
        return 0;
    chessboard[x][y] = mode;
    int dingmax, dingmin, dingnum;
    int doublethree = 0;
    
    int aiming = mode * 2;
    
    for (int p = -3; p < 1; p++)
    {
        dingmax = x + p + 3;
        dingmin = x + p - 1;
        if (dingmax >= border || dingmin < 0)
            continue;
        if (chessboard[dingmin][y] != 0 || chessboard[dingmax][y] != 0)
            continue;
        dingnum = 0;
        for (int i = x + p; i < x + p + 3; i++)
        {
            if (i < 0 || i >= border)
            {
                dingnum = 0;
                break;
            }
            dingnum += chessboard[i][y];
        }
        if (dingnum == aiming)
        {
            doublethree++;
            break;
        }
    }
    for (int p = -3; p < 1; p++)
    {
        dingmax = y + p + 3;
        dingmin = y + p - 1;
        if (dingmax >= border || dingmin < 0)
            continue;
        if (chessboard[x][dingmin] != 0 || chessboard[x][dingmax] != 0)
            continue;
        dingnum = 0;
        for (int i = y + p; i < y + p + 3; i++)
        {
            if (i < 0 || i >= border)
            {
                dingnum = 0;
                break;
            }
            dingnum += chessboard[x][i];
        }
        if (dingnum == aiming)
        {
            doublethree++;
            break;
        }
    }
    for (int p = -3; p < 1; p++)
    {
        dingmax = p + 3;
        dingmin = p - 1;
        if (x + dingmax >= border || x + dingmin < 0 || y + dingmax >= border || y + dingmin < 0)
            continue;
        if (chessboard[x + dingmin][y + dingmin] != 0 || chessboard[x + dingmax][y + dingmax] != 0)
            continue;
        dingnum = 0;
        for (int i = p; i < p + 3; i++)
        {
            if (x + i < 0 || x + i >= border || y + i < 0 || y + i >= border)
            {
                dingnum = 0;
                break;
            }
            dingnum += chessboard[x + i][y + i];
        }
        if (dingnum == aiming)
        {
            doublethree++;
            break;
        }
    }
    for (int p = -3; p < 1; p++)
    {
        dingmax = p + 3;
        dingmin = p - 1;
        if (x - dingmax < 0 || x - dingmin >= border || y + dingmax >= border || y + dingmin < 0)
            continue;
        if (chessboard[x - dingmin][y + dingmin] != 0 || chessboard[x - dingmax][y + dingmax] != 0)
            continue;
        dingnum = 0;
        for (int i = p; i < p + 3; i++)
        {
            if (x - i < 0 || x - i >= border || y + i < 0 || y + i >= border)
            {
                dingnum = 0;
                break;
            }
            dingnum += chessboard[x - i][y + i];
        }
        if (dingnum == aiming)
        {
            doublethree++;
            break;
        }
    }
    chessboard[x][y] = 0;
    return doublethree;
}
-(int) doublethreetest:(int) x y:(int) y mode:(int) mode type:(int) type border: (int) border;
{
    if (x < 0 || x >= border || y < 0 || y >= border)
        return 0;
    if (chessboard[x][y] != 0)
        return 0;
    chessboard[x][y] = mode;
    int dingmax, dingmin, dingnum;
    int doublethree = 0;
    int aiming = mode * 3;
    int typer = 0;
    for (int p = -4; p < 1; p++)
    {
        dingmax = x + p + 4;
        dingmin = x + p - 1;
        if (dingmax >= border || dingmin < 0)
            continue;
        if (chessboard[dingmin][y] != 0 || chessboard[dingmax][y] != 0)
            continue;
        dingnum = 0;
        for (int i = x + p; i < x + p + 4; i++)
        {
            if (i < 0 || i >= border)
            {
                dingnum = 0;
                break;
            }
            dingnum += chessboard[i][y];
        }
        if (dingnum == aiming)
        {
            doublethree++;
            typer = 1;
            break;
        }
    }
    for (int p = -4; p < 1; p++)
    {
        dingmax = y + p + 4;
        dingmin = y + p - 1;
        if (dingmax >= border || dingmin < 0)
            continue;
        if (chessboard[x][dingmin] != 0 || chessboard[x][dingmax] != 0)
            continue;
        dingnum = 0;
        for (int i = y + p; i < y + p + 4; i++)
        {
            if (i < 0 || i >= border)
            {
                dingnum = 0;
                break;
            }
            dingnum += chessboard[x][i];
        }
        if (dingnum == aiming)
        {
            doublethree++;
            typer = 2;
            break;
        }
    }
    for (int p = -4; p < 1; p++)
    {
        dingmax = p + 4;
        dingmin = p - 1;
        if (x + dingmax >= border || x + dingmin < 0 || y + dingmax >= border || y + dingmin < 0)
            continue;
        if (chessboard[x + dingmin][y + dingmin] != 0 || chessboard[x + dingmax][y + dingmax] != 0)
            continue;
        dingnum = 0;
        for (int i = p; i < p + 4; i++)
        {
            if (x + i < 0 || x + i >= border || y + i < 0 || y + i >= border)
            {
                dingnum = 0;
                break;
            }
            dingnum += chessboard[x + i][y + i];
        }
        if (dingnum == aiming)
        {
            doublethree++;
            typer = 3;
            break;
        }
    }
    for (int p = -4; p < 1; p++)
    {
        dingmax = p + 4;
        dingmin = p - 1;
        if (x - dingmax < 0 || x - dingmin >= border || y + dingmax >= border || y + dingmin < 0)
            continue;
        if (chessboard[x - dingmin][y + dingmin] != 0 || chessboard[x - dingmax][y + dingmax] != 0)
            continue;
        dingnum = 0;
        for (int i = p; i < p + 4; i++)
        {
            if (x - i < 0 || x - i >= border || y + i < 0 || y + i >= border)
            {
                dingnum = 0;
                break;
            }
            dingnum += chessboard[x - i][y + i];
        }
        if (dingnum == aiming)
        {
            doublethree++;
            typer = 4;
            break;
        }
    }
    chessboard[x][y] = 0;
    if (type == 1)
        return typer * 10 + doublethree;
    return doublethree;
}
-(int) doublethreetest_save:(int) x y:( int) y mode:(int) mode savers:(int[7]) saver border:(int) border;
{
    if (x < 0 || x >= border || y < 0 || y >= border)
        return 0;
    if (chessboard[x][y] != 0)
        return 0;
    chessboard[x][y] = mode;
    int blanker [2]={};
    int dingmax, dingmin, dingnum;
    int aiming = mode * 3;
    int blanknum = 0;
    
    for (int p = -4; p < 1; p++)
    {
        dingmax = x + p + 4;
        dingmin = x + p - 1;
        if (dingmax >= border || dingmin < 0)
            continue;
        if (chessboard[dingmin][y] != 0 || chessboard[dingmax][y] != 0)
            continue;
        dingnum = 0;
        for (int i = x + p; i < x + p + 4; i++)
        {
            if (i < 0 || i >= border)
            {
                dingnum = 0;
                break;
            }
            if (chessboard[i][y] == 0)
            {
                blanker[0] = i;
                blanker[1] = y;
            }
            else
            {
                dingnum += chessboard[i][y];
            }
            
        }
        if (dingnum == aiming)
        {
            
            
            if (chessboard[dingmin + 1][y] == mode || dingmax + 1 >= border || chessboard[dingmax + 1][y] == -mode)
            {
                saver[blanknum * 2] = dingmin;
                saver[blanknum * 2 + 1] = y;
                blanknum++;
            }
            
            saver[blanknum * 2] = blanker[0];
            saver[blanknum * 2 + 1] = blanker[1];
            blanknum++;
            
            if (chessboard[dingmax - 1][y] == mode || dingmin - 1 < 0 || chessboard[dingmin - 1][y] == -mode)
            {
                saver[blanknum * 2] = dingmax;
                saver[blanknum * 2 + 1] = y;
                blanknum++;
            }
            saver[6] = blanknum;
            
            chessboard[x][y] = 0;
            return 1;
        }
    }
    for (int p = -4; p < 1; p++)
    {
        dingmax = y + p + 4;
        dingmin = y + p - 1;
        if (dingmax >= border || dingmin < 0)
            continue;
        if (chessboard[x][dingmin] != 0 || chessboard[x][dingmax] != 0)
            continue;
        dingnum = 0;
        for (int i = y + p; i < y + p + 4; i++)
        {
            if (i < 0 || i >= border)
            {
                dingnum = 0;
                break;
            }
            if (chessboard[x][i] == 0)
            {
                blanker[0] = x;
                blanker[1] = i;
            }
            else
            {
                dingnum += chessboard[x][i];
            }
            
        }
        if (dingnum == aiming)
        {
            
            if (chessboard[x][ dingmin + 1] == mode || dingmax + 1 >= border || chessboard[x][dingmax + 1] == -mode)
            {
                saver[blanknum * 2] = x;
                saver[blanknum * 2 + 1] = dingmin;
                blanknum++;
            }
            
            saver[blanknum * 2] = blanker[0];
            saver[blanknum * 2 + 1] = blanker[1];
            blanknum++;
            if (chessboard[x][dingmax - 1] == mode || dingmin - 1 < 0 || chessboard[x][dingmin - 1] == -mode)
            {
                saver[blanknum * 2] = x;
                saver[blanknum * 2 + 1] = dingmax;
                blanknum++;
            }
            saver[6] = blanknum;
            
            chessboard[x][ y] = 0;
            return 1;
        }
    }
    for (int p = -4; p < 1; p++)
    {
        dingmax = p + 4;
        dingmin = p - 1;
        if (x + dingmax >= border || x + dingmin < 0 || y + dingmax >= border || y + dingmin < 0)
            continue;
        if (chessboard[x + dingmin][y + dingmin] != 0 || chessboard[x + dingmax][y + dingmax] != 0)
            continue;
        dingnum = 0;
        for (int i = p; i < p + 4; i++)
        {
            if (x + i < 0 || x + i >= border || y + i < 0 || y + i >= border)
            {
                dingnum = 0;
                break;
            }
            if (chessboard[x + i][y + i] == 0)
            {
                blanker[0] = x + i;
                blanker[1] = y + i;
            }
            else
            {
                dingnum += chessboard[x + i][ y + i];
            }
            
        }
        if (dingnum == aiming)
        {
            
            
            if (chessboard[x + dingmin + 1][ y + dingmin + 1] == mode || x + dingmax + 1 >= border || y + dingmax + 1 >= border || chessboard[x + dingmax + 1][y + dingmax + 1] == -mode)
            {
                saver[blanknum * 2] = x + dingmin;
                saver[blanknum * 2 + 1] = y + dingmin;
                blanknum++;
            }
            
            saver[blanknum * 2] = blanker[0];
            saver[blanknum * 2 + 1] = blanker[1];
            blanknum++;
            if (chessboard[x + dingmax - 1][ y + dingmax - 1] == mode || x + dingmin - 1 < 0 || y + dingmin - 1 < 0 || chessboard[x + dingmin - 1][y + dingmin - 1] == -mode)
            {
                saver[blanknum * 2] = x + dingmax;
                saver[blanknum * 2 + 1] = y + dingmax;
                blanknum++;
            }
            saver[6] = blanknum;
            
            chessboard[x][y] = 0;
            return 1;
        }
    }
    for (int p = -4; p < 1; p++)
    {
        dingmax = p + 4;
        dingmin = p - 1;
        if (x - dingmax < 0 || x - dingmin >= border || y + dingmax >= border || y + dingmin < 0)
            continue;
        if (chessboard[x - dingmin][ y + dingmin] != 0 || chessboard[x - dingmax][ y + dingmax] != 0)
            continue;
        dingnum = 0;
        for (int i = p; i < p + 4; i++)
        {
            if (x - i < 0 || x - i >= border || y + i < 0 || y + i >= border)
            {
                dingnum = 0;
                break;
            }
            if (chessboard[x - i][y + i] == 0)
            {
                blanker[0] = x - i;
                blanker[1] = y + i;
            }
            else
            {
                dingnum += chessboard[x - i][y + i];
            }
            
        }
        if (dingnum == aiming)
        {
            if (chessboard[x - dingmin - 1][y + dingmin + 1] == mode || x - dingmax - 1 < 0 || y + dingmax + 1 >= border || chessboard[x - dingmax - 1][y + dingmax + 1] == -mode)
            {
                saver[blanknum * 2] = x - dingmin;
                saver[blanknum * 2 + 1] = y + dingmin;
                blanknum++;
            }
            
            saver[blanknum * 2] = blanker[0];
            saver[blanknum * 2 + 1] = blanker[1];
            blanknum++;
            if (chessboard[x - dingmax + 1][y + dingmax - 1] == mode || x - dingmin + 1 >= border || y + dingmin - 1 < 0 || chessboard[x - dingmin + 1][y + dingmin - 1] == -mode)
            {
                saver[blanknum * 2] = x - dingmax;
                saver[blanknum * 2 + 1] = y + dingmax;
                blanknum++;
            }
            saver[6] = blanknum;
            
            chessboard[x][y] = 0;
            return 1;
        }
    }
    chessboard[x][y] = 0;
    return 0;
}

-(int) killpoint:(int) x y:(int) y mode:(int) mode;
{
    
    int threepoint_white = [self doublethreetest:x y:y mode:mode type:1 border:15];
    int direct_three = threepoint_white / 10;
    int num_three = threepoint_white % 10;
    int keywhite =[self keypoint:x y:y mode:mode type:1];
    int direct_key = keywhite / 10;
    int num_key = keywhite % 10;
    
    if (banned_mode == 1 && mode == 1)
    {
        if ([self banned_point:x j:y] != 0)
            return 0;
    }
    
    if (num_key > 1)// double four condition
        return 1;
    
    if (num_key > 0 && (num_three > 1 || num_three == 1 && direct_key != direct_three))// four and three condition
    {
        int fighter[2]={};
        [self keypoint_save:x y:y mode:mode aimer:fighter];
        
        int fighter_number = [self keypoint:fighter[0] y:fighter[1] mode:-mode type:0];
        if (fighter_number == 0) // no way to fight back
        {
            return 1;
        }
        else
        {
            if (fighter_number > 1) // no way to defense rival's attack
            {
                return 0;
            }
            chessboard[x][y] = mode;  // if put in current chess at this point
            
            int refighter [2]={};
            [self keypoint_save:fighter[0] y:fighter[1] mode:mode aimer:refighter];// search fight back position
            
            chessboard[fighter[0] ][fighter[1]] = -mode;
            if ([self killpoint:refighter[0] y:refighter[1] mode:mode]  == 1) // if fight back position is still killpoint then return 1
            {
                chessboard[x][y] = 0;
                chessboard[fighter[0] ][fighter[1]] = 0;
                return 1;
            }
            chessboard[x][y] = 0;
            chessboard[fighter[0]][ fighter[1]] = 0;
        }
    }
    return 0;
}
-(int) add_a_chess:(int) pl_x pl_y:(int) pl_y mode:(int) mode;
{
    if(mode==-1)
    {
        if([self checkidea:pl_x y:pl_y mode:-1 scale:5 border:15]!=0)
        {
            game_end=-1;
        }
    }
    else{
        if([self checkidea:pl_x y:pl_y mode:1 scale:5 border:15]!=0)
        {
            game_end=1;
        }
    }
    
    if(mode==-1)
    {
        chessboard[pl_x][pl_y]=-1;
    }
    else{
        chessboard[pl_x][pl_y]=1;
    }
    process[chessid][0]=pl_x;
    process[chessid][1]=pl_y;
    last_pos[0]=pl_x;
    last_pos[1]=pl_y;
    last_color=mode;
    
    chessid++;
    return game_end;
}
-(void) set_banmode:(int)mode
{
    banned_mode=mode;
}
-(void) row_add_a_chess:(int) pl_x pl_y:(int) pl_y mode:(int) mode;
{
    chessid++;
    if([self checkidea:pl_x y:pl_y mode:mode scale:5 border:15]!=0)
    {
        game_end=mode;
    }
    chessboard[pl_x][pl_y]=mode;
}
//analysis
-(void) egg_analysisboard:(int) mode
{
    for (int i = 0; i < 15; i++)
        for (int j = 0; j < 15; j++)
        {
            if (chessboard[i][j] != 0)
            {
                enemy_force_map[i][j] = false;
                continue;
            }
            if ([self doublethreetest:i y:j mode:-mode type:0 border:15] != 0 || [self keypoint:i y:j mode:-mode type:0] != 0)
            {
                enemy_force_map[i][j] = true;
            }
            else
            {
                enemy_force_map[i][j] = false;
            }
        }
    
    [self update_border];
    
    int adv_analysis_res = [self egg_adv_analysis:mode];
    
    if (adv_analysis_res == 1)
    {
        [self update_border];
        return;
    }
    
    if (totalprocess >= 220)
    {
        game_end = -1;
        return;
    }
    
    int pl_score = -1000000;
    int pl_x = 0, pl_y = 0;
    
    for (int i = border_x_min; i < border_x_max + 1; i++)
    {
        if (i < 0 || i > maxchessline - 1)
            continue;
        
        for (int j = border_y_min; j < border_y_max + 1; j++)
        {
            if (j > maxchessline - 1 || j < 0)
                continue;
            
            if (chessboard[i][j] != 0)
                continue;
            
            int A_score = [self egg_point_score:i y:j mode:mode];
            
            if (i == border_x_min || i == border_x_max|| j == border_y_min || j == border_y_max)
                A_score -= 1000;
            if (i == 0 || i == maxchessline - 1 || j == 0 || j == maxchessline - 1)
                A_score -= 10000;
            
            if (A_score > pl_score + 1)
            {
                
                pl_score = A_score;
                pl_x = i;
                pl_y = j;
                
            }
            
        }
        
    }
    
    if ([self checkidea:pl_x y:pl_y mode:mode scale:5 border:15] != 0)
    {
        game_end = mode;
        //game_end=1;
    }
    [self emoji_techer:pl_score];
    [self add_a_chess:pl_x pl_y:pl_y mode:mode];
    
}
-(void) harsh_analysisboard:(int) mode
{
    for (int i = 0; i < 15; i++)
        for (int j = 0; j < 15; j++)
        {
            if (chessboard[i][j] != 0)
            {
                enemy_force_map[i][j] = false;
                continue;
            }
            if ([self doublethreetest:i y:j mode:-mode type:0 border:15] != 0 || [self keypoint:i y:j mode:-mode type:0] != 0)
            {
                enemy_force_map[i][j] = true;
            }
            else
            {
                enemy_force_map[i][j] = false;
            }
        }
    
    
    [self update_border];
    //now_tech=@"";
    int adv_analysis_res=[self harsh_adv_analysis:mode];
    
    if(adv_analysis_res==1)
    {
        return;
    }
    
    if(chessid>=225)
    {
        game_end=1;
        return ;
    }
    
    double pl_score=-100000;
    int pl_x=0,pl_y=0;
    
    
    for(int i=border_x_min;i<border_x_max+1;i++)
    {
        if(i<0||i>maxchessline-1)
            continue;
        
        for(int j=border_y_min;j<border_y_max+1;j++)
        {
            if(j>maxchessline-1||j<0)
                continue;
            
            if(chessboard[i][j]!=0)
                continue;
            
            double A_score=[self point_score:i y:j mode:mode];
            if (i == border_x_min || i == border_x_max|| j == border_y_min || j == border_y_max)
                A_score -= 1000;
            if(i==0||i==maxchessline-1||j==0||j==maxchessline-1)
                A_score-=10000;
            
            if(A_score>pl_score+1)
            {
                pl_score=A_score;
                pl_x=i;
                pl_y=j;
            }
            
        }
        
    }
    [self emoji_techer:pl_score];
    
    [self add_a_chess:pl_x pl_y:pl_y mode:mode];
}
-(void) easy_analysisboard:(int) mode
{
    for (int i = 0; i < 15; i++)
        for (int j = 0; j < 15; j++)
        {
            if (chessboard[i][j] != 0)
            {
                enemy_force_map[i][j] = false;
                continue;
            }
            if ([self doublethreetest:i y:j mode:-mode type:0 border:15] != 0 || [self keypoint:i y:j mode:-mode type:0] != 0)
            {
                enemy_force_map[i][j] = true;
            }
            else
            {
                enemy_force_map[i][j] = false;
            }
        }
    
    [self update_border];
    int adv_analysis_res=[self easy_adv_analysis:mode];
    if(adv_analysis_res==1)
    {
        return;
    }
    
    if(chessid>=225)
    {
        game_end=1;
        return ;
    }
    
    double pl_score=-100000;
    int pl_x=0,pl_y=0;
    
    
    for(int i=border_x_min;i<border_x_max+1;i++)
    {
        if(i<0||i>maxchessline-1)
            continue;
        
        for(int j=border_y_min;j<border_y_max+1;j++)
        {
            if(j>maxchessline-1||j<0)
                continue;
            
            if(chessboard[i][j]!=0)
                continue;
            
            double A_score=[self easy_point_score:i y:j mode:mode];
            if(A_score>3000)
                A_score -= rand()%1000;
            else
            {
                A_score -= rand()%200;
            }
            
            if(i==border_x_min||i==border_x_max||j==border_y_min||j==border_y_max)
                A_score-=2000;
            if(i==0||i==maxchessline-1||j==0||j==maxchessline-1)
                A_score-=10000;
            
            if(A_score>pl_score+1)
            {
                pl_score=A_score;
                pl_x=i;
                pl_y=j;
            }
            
        }
        
    }
    [self emoji_techer:pl_score];
    [self add_a_chess:pl_x pl_y:pl_y mode:mode];
}
-(void)emoji_techer:(double)sc
{
    if(sc<-8000)
        now_tech=[[NSString alloc]initWithFormat:@"😱😱"];
    else if(sc<-1000)
        now_tech=[[NSString alloc]initWithFormat:@"😨😰"];
    else if(sc<5000)
        now_tech=[[NSString alloc]initWithFormat:@"😶😶"];
    else if(sc<20000)
        now_tech=[[NSString alloc]initWithFormat:@"🤔🤭"];
    else if(sc<40000)
        now_tech=[[NSString alloc]initWithFormat:@"😯😯"];
    else
        now_tech=[[NSString alloc]initWithFormat:@"😛😝"];
}
-(int) ban_keypoint:(int) x y:(int) y mode:(int) mode direct:(bool[]) direct type:(int) type
{
    if (x < 0 || x > maxchessline - 1 || y < 0 || y > maxchessline - 1)
        return 0;
    if (chessboard[x][y] != 0)
        return 0;
    
    chessboard[x][y] = mode;
    for(int i=0;i<5;i++)
        direct[i]=false;
    
    int blanker[2] ;
    int oldblanker[2] ;
    for (int i = 0; i < 2; i++)
    {
        blanker[i] = -1;
        oldblanker[i] = -1;
    }
    int oldexist = 0;
    int tid = 0;
    int typer = 0;
    for (int d = x - 5; d < x + 1; d++)
    {
        int counter = 0;
        for (int j = d; j < d + 5; j++)
        {
            if (j < 0 || j > maxchessline - 1)
            {
                counter = 0;
                break;
            }
            if (chessboard[j][y] == -mode)
            {
                counter = 0;
                break;
            }
            if (chessboard[j][y] == 0)
            {
                blanker[0] = j;
                blanker[1] = y;
                counter++;
            }
            
        }
        
        if (counter == 1)
        {
            if (oldexist == 0 || blanker[0] != oldblanker[0] || blanker[1] != oldblanker[1])
                tid++;
            
            if (oldexist == 0)
            {
                oldblanker[0] = blanker[0];
                oldblanker[1] = blanker[1];
                oldexist = 1;
            }
            typer = 1;
            direct[1] = true;
        }
    }
    for (int d = y - 5; d < y + 1; d++)
    {
        int counter = 0;
        for (int j = d; j < d + 5 && counter < 2; j++)
        {
            if (j < 0 || j > maxchessline - 1)
            {
                counter = 0;
                break;
            }
            if (chessboard[x][j] == -mode)
            {
                counter = 0;
                break;
            }
            if (chessboard[x][j] == 0)
            {
                counter++;
                blanker[0] = x;
                blanker[1] = j;
            }
            
        }
        
        if (counter == 1)
        {
            if (oldexist == 0 || blanker[0] != oldblanker[0] || blanker[1] != oldblanker[1])
                tid++;
            
            if (oldexist == 0)
            {
                oldblanker[0] = blanker[0];
                oldblanker[1] = blanker[1];
                oldexist = 1;
            }
            typer = 2;
            direct[2] = true;
        }
    }
    
    for (int d = -5; d < 1; d++)
    {
        int counter = 0;
        for (int j = d; j < d + 5 && counter < 2; j++)
        {
            if (x + j < 0 || x + j > maxchessline - 1 || y + j < 0 || y + j > maxchessline - 1)
            {
                counter = 0;
                break;
            }
            if (chessboard[x + j][ y + j] == -mode)
            {
                counter = 0;
                break;
            }
            if (chessboard[x + j][ y + j] == 0)
            {
                counter++;
                blanker[0] = x + j;
                blanker[1] = y + j;
            }
            
        }
        
        if (counter == 1)
        {
            if (oldexist == 0 || blanker[0] != oldblanker[0] || blanker[1] != oldblanker[1])
                tid++;
            
            if (oldexist == 0)
            {
                oldblanker[0] = blanker[0];
                oldblanker[1] = blanker[1];
                oldexist = 1;
            }
            typer = 3;
            direct[3] = true;
        }
    }
    for (int d = -5; d < 1; d++)
    {
        int counter = 0;
        for (int j = d; j < d + 5 && counter < 2; j++)
        {
            if (x - j < 0 || x - j > maxchessline - 1 || y + j < 0 || y + j > maxchessline - 1)
            {
                counter = 0;
                break;
            }
            if (chessboard[x - j][y + j] == -mode)
            {
                counter = 0;
                break;
            }
            if (chessboard[x - j][ y + j] == 0)
            {
                counter++;
                blanker[0] = x - j;
                blanker[1] = y + j;
            }
            
        }
        
        if (counter == 1)
        {
            if (oldexist == 0 || blanker[0] != oldblanker[0] || blanker[1] != oldblanker[1])
                tid++;
            
            if (oldexist == 0)
            {
                oldblanker[0] = blanker[0];
                oldblanker[1] = blanker[1];
                oldexist = 1;
            }
            typer = 4;
            direct[4] = true;
        }
    }
    
    chessboard[x][y] = 0;
    return tid;
}
-(int) banned_point:(int) i j:(int) j
{
    
    bool direct[5]={};
    int ban_key = [self ban_keypoint:i y:j mode:1 direct:direct type:0];
    if (ban_key > 1)
    {
        int alldirect = 0;
        for (int q = 1; q < 5; q++)
        {
            if (direct[q] == true)
            {
                alldirect++;
            }
        }
        if (alldirect != 1)
            return 1;
    }
    
    if ([self doublethreetest:i y:j mode:1 type:0 border:15] > 1 && ban_key == 0)
        return 1;
    
    return 0;
}
-(int) easy_adv_analysis:(int) mode;
{
    // Rank 1 attack
    int dead[100][2] ;
    int dead_attack_total =[self dead_point:dead mode:mode];
    if (dead_attack_total != 0)
    {
        // cout<<"adv attack type 1"<<endl;
        [self add_a_chess:dead[0][0] pl_y:dead[0][1] mode:mode];
        game_end = mode;
        now_tech = @"☀️☀️";
        return 1;
    }
    // Rank 1 defense
    int dead_defense_total = [self dead_point:dead mode:-mode];
    if (dead_defense_total != 0)
    {
        if (banned_mode == 1 && mode == 1)
        {
            if ([self banned_point:dead[0][0] j:dead[0][1]] == 1)
            {
                game_end = -1;
            }
        }
        // cout<<"adv defense type 1"<<endl;
        [self add_a_chess:dead[0][0] pl_y:dead[0][1] mode:mode];
        
        now_tech = @"🌧🌧";
        return 1;
    }
    
    // Rank 2 attack
    for (int i = border_x_min; i < border_x_max + 1; i++)
        for (int j = border_y_min; j < border_y_max; j++)
        {
            if ([self killpoint:i y:j mode:mode] == 1)
            {
                [self add_a_chess:i pl_y:j mode:mode];
                now_tech = @"🌤🌤";
                return 1;
            }
        }
    
    // Rank 2 pre attack and defense
    int fast_attack [100][2];
    int fast_res = [self fast_attack_point:fast_attack mode:mode];
    if (fast_res > 0)
    {
        int high_score = -1000000;
        int high_id = -1;
        for (int i = 0; i < fast_res; i++)
        {
            //   if (fight_back_level(fast_attack[i, 0], fast_attack[i, 1], mode) != 0)
            //    continue;
            
            int current_score = [self easy_point_score:fast_attack[i][0] y:fast_attack[i][1] mode:mode];
            if (current_score > high_score)
            {
                high_score = current_score;
                high_id = i;
            }
        }
        if (high_id != -1)
        {
            [self add_a_chess:fast_attack[high_id][0] pl_y:fast_attack[high_id][1] mode:mode];
            
            now_tech = @"⚡️⚡️";
            return 1;
        }
        
    }
    
    // Rank 2 defense
    // decicive
    
    int need_defense [100][2];
    int enemy_force [100][2];
    int enemy_force_total = [self force_point:enemy_force mode: -mode];
    int need_defense_total = [self need_point:need_defense mode: mode];
    
    if (enemy_force_total > 0)
    {
        if (need_defense_total == 0)
            need_defense_total = [self dec_need_point:need_defense mode:mode];
        
        if (need_defense_total == 0)
        {
            int guess_defense_id = rand()% enemy_force_total;
            int dinger = -1;
            for (int i = 0; i < enemy_force_total; i++)
                if ([self keypoint:enemy_force[i][0] y:enemy_force[i][1] mode:mode type:0] != 0)
                {
                    dinger = i;
                    break;
                }
            if (dinger == -1)
                [self add_a_chess:enemy_force[guess_defense_id][0] pl_y:enemy_force[guess_defense_id][1] mode:mode];
            else
            {
                [self add_a_chess:enemy_force[dinger][0] pl_y:enemy_force[dinger][1] mode:mode];
            }
            now_tech = @"❄️❄️";
            return 1;
        }
        int high_id = -1;
        int high_score = -100000;
        
        for (int p = 0; p < need_defense_total; p++)
        {
            
            int current_score = [self easy_point_score:need_defense[p][0] y:need_defense[p][1] mode:mode];
            
            if (current_score > high_score)
            {
                high_id = p;
                high_score = current_score;
            }
        }
        if (high_id == -1)
            high_id = rand()% need_defense_total;
        
        if (banned_mode == 1 && mode == 1)
        {
            if ([self banned_point:need_defense[high_id][0] j:need_defense[high_id][1]] == 1)
            {
                game_end = -1;
            }
        }
        [self add_a_chess:need_defense[high_id][0] pl_y:need_defense[high_id][1] mode:mode];
        now_tech = [[NSString alloc]initWithFormat: @"⛈⛈"];
        return 1;
    }
    
    return 0;
}
-(int) egg_adv_analysis:(int) mode;
{
    // Rank 1 attack
    int dead[100][2] ;
    int dead_attack_total =[self dead_point:dead mode:mode];
    if (dead_attack_total != 0)
    {
        // cout<<"adv attack type 1"<<endl;
        [self add_a_chess:dead[0][0] pl_y:dead[0][1] mode:mode];
        game_end = mode;
        now_tech = @"😁😁";
        return 1;
    }
    // Rank 1 defense
    int dead_defense_total = [self dead_point:dead mode:-mode];
    if (dead_defense_total != 0)
    {
        if (banned_mode == 1 && mode == 1)
        {
            if ([self banned_point:dead[0][0] j:dead[0][1]] == 1)
            {
                game_end = -1;
            }
        }
        // cout<<"adv defense type 1"<<endl;
        [self add_a_chess:dead[0][0] pl_y:dead[0][1] mode:mode];
        
        now_tech = @"😖😖";
        return 1;
    }
    
    // Rank 2 attack
    for (int i = border_x_min; i < border_x_max + 1; i++)
        for (int j = border_y_min; j < border_y_max; j++)
        {
            if ([self killpoint:i y:j mode:mode] == 1)
            {
                [self add_a_chess:i pl_y:j mode:mode];
                now_tech = @"😏😏";
                return 1;
            }
        }
    
    // Rank 2 pre attack and defense
    int fast_attack [100][2];
    int fast_res = [self fast_attack_point:fast_attack mode:mode];
    if (fast_res > 0)
    {
        int high_score = -1000000;
        int high_id = -1;
        for (int i = 0; i < fast_res; i++)
        {
            //   if (fight_back_level(fast_attack[i, 0], fast_attack[i, 1], mode) != 0)
            //    continue;
            
            int current_score = [self egg_point_score:fast_attack[i][0] y:fast_attack[i][1] mode:mode];
            if (current_score > high_score)
            {
                high_score = current_score;
                high_id = i;
            }
        }
        if (high_id != -1)
        {
            [self add_a_chess:fast_attack[high_id][0] pl_y:fast_attack[high_id][1] mode:mode];
            
            now_tech = @"😮😮";
            return 1;
        }
        
    }
    
    // Rank 2 defense
    // decicive
    
    int need_defense [100][2];
    int enemy_force [100][2];
    int enemy_force_total = [self force_point:enemy_force mode: -mode];
    int need_defense_total = [self need_point:need_defense mode: mode];
    
    if (enemy_force_total > 0)
    {
        if (need_defense_total == 0)
            need_defense_total = [self dec_need_point:need_defense mode:mode];
        
        if (need_defense_total == 0)
        {
            int guess_defense_id = rand()% enemy_force_total;
            int dinger = -1;
            for (int i = 0; i < enemy_force_total; i++)
                if ([self keypoint:enemy_force[i][0] y:enemy_force[i][1] mode:mode type:0] != 0)
                {
                    dinger = i;
                    break;
                }
            if (dinger == -1)
                [self add_a_chess:enemy_force[guess_defense_id][0] pl_y:enemy_force[guess_defense_id][1] mode:mode];
            else
            {
                [self add_a_chess:enemy_force[dinger][0] pl_y:enemy_force[dinger][1] mode:mode];
            }
            now_tech = @"😱😱";
            return 1;
        }
        int high_id = -1;
        int high_score = -100000;
        
        for (int p = 0; p < need_defense_total; p++)
        {
            
            int current_score = [self egg_point_score:need_defense[p][0] y:need_defense[p][1] mode:mode];
            
            if (current_score > high_score)
            {
                high_id = p;
                high_score = current_score;
            }
        }
        if (high_id == -1)
            high_id = rand()% need_defense_total;
        
        if (banned_mode == 1 && mode == 1)
        {
            if ([self banned_point:need_defense[high_id][0] j:need_defense[high_id][1]] == 1)
            {
                game_end = -1;
            }
        }
        [self add_a_chess:need_defense[high_id][0] pl_y:need_defense[high_id][1] mode:mode];
        now_tech = [[NSString alloc]initWithFormat: @"😥😥"];
        return 1;
    }
    
    return 0;
}
-(int) fight_back_level:(int) x y:(int) y mode:(int) mode;
{
    if (chessboard[x][y] != 0)
        return 0;
    
    int rival_have_to_go[2]={};
    int rival_may_go[7]={};
    int keyer=[self keypoint_save:x y:y mode:mode aimer:rival_have_to_go];
    int threer=[self doublethreetest_save:x y:y mode:mode savers:rival_may_go border:15];
    int threenum =[self doublethreetest:x y:y mode:mode type:0 border:15];
    if([self killpoint:x y:y mode:mode]==1)
        return 0;
    
    chessboard[x][y]=mode;
    
    int may_res=rival_may_go[6];
    
    if(keyer==0&&may_res==0)
    {
        chessboard[x][y]=0;
        return 0;
    }
    
    if(keyer>0)
    {
        
        if (threer==0&& [self doublethreetest:rival_have_to_go[0] y:rival_have_to_go[1] mode:-mode type:0 border:15] != 0)
        {
            chessboard[x][y] = 0;
            return 1;
        }
        
        chessboard[x][y] = 0;
        return 0;
    }
    else{
        for (int p=0;p<may_res;p++)
        {
            if(threenum==1&&[self doublethreetest:rival_may_go[p*2] y:rival_may_go[p*2+1] mode:-mode type:0 border:15] !=0)
            {
                chessboard[x][y] = 0;
                return 1;
            }
        }
    }
    
    chessboard[x][y]=0;
    return 0;
}
-(int) harsh_fast_attack_point:(int[100][2])attackers mode:(int) mode;
{
    int faster=0;
    for (int i=border_x_min;i<border_x_max+1&&faster==0;i++)
        for(int j=border_y_min;j<border_y_max+1;j++)
        {
            if (i < 0 || i > 18 || j < 0 || j > 18)
                continue;
            if(chessboard[i][j]!=0)
                continue;
            
            if ([self harsh_super_fast_attack:i y:j mode:mode tower:0] != 0 )
            {
                attackers[faster][0] = i;
                attackers[faster][1] = j;
                faster++;
                continue;
            }
        }
    return faster;
}
-(int) fast_attack_point:(int[100][2])attackers mode:(int) mode;
{
    int faster=0;
    for (int i=border_x_min;i<border_x_max+1&&faster==0;i++)
        for(int j=border_y_min;j<border_y_max+1;j++)
        {
            if (i < 0 || i > 18 || j < 0 || j > 18)
                continue;
            if(chessboard[i][j]!=0)
                continue;
            
            if ([self harsh_super_fast_attack:i y:j mode:mode tower:0] != 0)
            {
                attackers[faster][0] = i;
                attackers[faster][1] = j;
                faster++;
                continue;
            }
        }
    return faster;
}
-(int) dead_point:(int[100][2])saver mode:(int)mode;
{
    int savenum = 0;
    for (int i = border_x_min; i < border_x_max + 1; i++)
    {
        if (i < 0 || i > maxchessline - 1)
            continue;
        
        for (int j = border_y_min; j < border_y_max + 1; j++)
        {
            if (j > maxchessline - 1 || j < 0)
                continue;
            if (chessboard[i][j] != 0)
                continue;
            
            if ([self checkidea:i y:j mode:mode scale:5 border:15]!= 0)
            {
                if (banned_mode == 0 || mode == -1)
                {
                    saver[savenum][0] = i;
                    saver[savenum][1] = j;
                    savenum++;
                }
                else
                {
                    if ([self checkidea:i y:j mode:mode scale:6 border:15]== 0)
                    {
                        saver[savenum][0] = i;
                        saver[savenum][1] = j;
                        savenum++;
                    }
                }
            }
            if (savenum > 25)
                return savenum;
        }
    }
    return savenum;
}
-(int) exist_sudden_fight_back:(int) x y:(int) y mode:(int) mode;
{
    chessboard[x][y] = mode;
    for (int i = border_x_min; i < border_x_max; i++)
    {
        if (i < 0 || i > maxchessline - 1)
            continue;
        
        for (int j = border_y_min; j < border_y_max; j++)
        {
            
            if (j > maxchessline - 1 || j < 0 || j == y && i == x)
                continue;
            if (enemy_force_map[i][j] == false)
                continue;
            //  chessboard[i, j] = -mode;
            if ([self imagine:i y:j mode:mode tower:0]!= 0)
            {
                //     print("find fight back at " + i + "," + j);
                chessboard[x][y] = 0;
                chessboard[i][j] = 0;
                return 1;
            }
            //   chessboard[i, j] = 0;
        }
    }
    chessboard[x][y] = 0;
    return 0;
}
-(int) exist_force:(int) mode;
{
    for(int i=border_x_min;i<border_x_max+1;i++)
    {
        if(i<0||i>maxchessline-1)
            continue;
        
        for(int j=border_y_min;j<border_y_max+1;j++)
        {
            if(j>maxchessline-1||j<0)
                continue;
            if(chessboard[i][j]!=0)
                continue;
            if([self harsh_super_fast_attack:i y:j mode:mode tower:0]!=0)
            {
                return 1;
            }
        }
    }
    return 0;
}
-(int) exist_imagine:(int) mode;
{
    for(int i=border_x_min;i<border_x_max+1;i++)
    {
        if(i<0||i>maxchessline-1)
            continue;
        
        for(int j=border_y_min;j<border_y_max+1;j++)
        {
            if(j>maxchessline-1||j<0)
                continue;
            if(chessboard[i][j]!=0)
                continue;
            if (enemy_force_map[i][j]==false)
                continue;
            if([self imagine:i y:j mode:-mode tower:2]!=0)
                return 1;
        }
    }
    return 0;
}
-(int) future_attack_possiblity:(int)x y:(int)y mode:(int)mode
{
    int acu=0;
    
    for(int i=border_x_min;i<border_x_max;i++)
    {
        if(i>maxchessline-1||i<0)
            continue;
        for(int j=border_x_min;j<border_x_max;j++)
        {
            if(j>maxchessline-1||j<0||j==y&&i==x)
                continue;
            if(chessboard[i][j]!=0)
                continue;
            if ([self doublethreetest:i y:j mode:mode type:0 border:15] == 0 && [self keypoint:i y:j mode:mode type:0] == 0)
                continue;
            if([self imagine:x y:y mode:mode tower:2]!=0)
                acu++;
        }
    }
    
    return acu;
}
-(int) force_point:(int[100][2])saver mode:(int) mode;
{
    int savenum=0;
    for(int i=border_x_min-2;i<border_x_max+3;i++)
    {
        if(i<0||i>maxchessline-1)
            continue;
        
        for(int j=border_y_min-2;j<border_y_max+3;j++)
        {
            if(j>maxchessline-1||j<0)
                continue;
            if(chessboard[i][j]!=0)
                continue;
            if ([self killpoint:i y:j mode:mode] != 0 || [self harsh_super_fast_attack:i y:j mode:mode tower:0] != 0 )
            {
                saver[savenum][0] = i;
                saver[savenum][1] = j;
                savenum++;
            }
        }
    }
    return savenum;
}

-(int) need_point:(int[100][2])saver mode:(int) mode;
{
    int force_defense [100][2]={};
    int force_defense_total = [self force_point:force_defense mode:-mode];
    if (force_defense_total == 0)
        return -1;
    
    int savenum = 0;
    for (int i = border_x_min; i < border_x_max + 1; i++)
    {
        if (i < 0 || i > maxchessline - 1)
            continue;
        
        for (int j = border_y_min; j < border_y_max + 1; j++)
        {
            if (j > maxchessline - 1 || j < 0)
                continue;
            if (chessboard[i][j] != 0)
                continue;
            int allclear = 1;
            chessboard[i][j] = mode;
            for (int p = 0; p < force_defense_total; p++)
            {
                if ([self killpoint:force_defense[p][0] y:force_defense[p][1] mode:-mode]  != 0 ||
                    [self harsh_super_fast_attack:force_defense[p][0] y:force_defense[p][1] mode:-mode tower:0 ]!= 0)
                {
                    allclear = 0;
                    break;
                }
                
            }
            if (allclear == 1)
            {
                saver[savenum][0] = i;
                saver[savenum][1] = j;
                savenum++;
            }
            chessboard[i][j] = 0;
            if (savenum > 100)
                return savenum;
        }
    }
    return savenum;
}
-(int) dec_force_point:(int[100][2])saver mode:(int) mode;
{
    int savenum=0;
    for(int i=border_x_min-2;i<border_x_max+3;i++)
    {
        if(i<0||i>maxchessline-1)
            continue;
        
        for(int j=border_y_min-2;j<border_y_max+3;j++)
        {
            if(j>maxchessline-1||j<0)
                continue;
            if(chessboard[i][j]!=0)
                continue;
            if ([self killpoint:i y:j mode:mode] != 0 )
            {
                saver[savenum][0] = i;
                saver[savenum][1] = j;
                savenum++;
            }
        }
    }
    return savenum;
}

-(int) dec_need_point:(int[100][2])saver mode:(int) mode;
{
    int force_defense [100][2]={};
    int force_defense_total = [self dec_force_point:force_defense mode:-mode];
    if (force_defense_total == 0)
        return -1;
    
    int savenum = 0;
    for (int i = border_x_min; i < border_x_max + 1; i++)
    {
        if (i < 0 || i > maxchessline - 1)
            continue;
        
        for (int j = border_y_min; j < border_y_max + 1; j++)
        {
            if (j > maxchessline - 1 || j < 0)
                continue;
            if (chessboard[i][j] != 0)
                continue;
            int allclear = 1;
            chessboard[i][j] = mode;
            for (int p = 0; p < force_defense_total; p++)
            {
                if ([self killpoint:force_defense[p][0] y:force_defense[p][1] mode:-mode]  != 0 )
                {
                    allclear = 0;
                    break;
                }
                
            }
            if (allclear == 1)
            {
                saver[savenum][0] = i;
                saver[savenum][1] = j;
                savenum++;
            }
            chessboard[i][j] = 0;
            if (savenum > 100)
                return savenum;
        }
    }
    return savenum;
}

-(int) double_three_point:(int[100][2])saver mode:(int) mode;
{
    int savenum=0;
    
    for(int i=border_x_min;i<border_x_max+1;i++)
    {
        if(i<0||i>maxchessline-1)
            continue;
        
        for(int j=border_y_min;j<border_y_max+1;j++)
        {
            if(j>maxchessline-1||j<0)
                continue;
            if(chessboard[i][j]!=0)
                continue;
            
            if([self imagine:i y:j mode:mode tower:0] !=0)
            {
                saver[savenum][0]=i;
                saver[savenum][1]=j;
                savenum++;
            }
            if(savenum>55)
                return savenum;
        }
    }
    
    return savenum;
}
-(int) harsh_adv_analysis:(int)mode
{
    // Rank 1 attack
    
    int dead[100][2] ;
    int dead_attack_total =[self dead_point:dead mode:mode];
    if (dead_attack_total != 0)
    {
        // cout<<"adv attack type 1"<<endl;
        [self add_a_chess:dead[0][0] pl_y:dead[0][1] mode:mode];
        game_end = mode;
        now_tech = @"🐠🐠";
        return 1;
    }
    // Rank 1 defense
    int dead_defense_total = [self dead_point:dead mode:-mode];
    if (dead_defense_total != 0)
    {
        if (banned_mode == 1 && mode == 1)
        {
            if ([self banned_point:dead[0][0] j:dead[0][1]] == 1)
            {
                game_end = -1;
            }
        }
        // cout<<"adv defense type 1"<<endl;
        [self add_a_chess:dead[0][0] pl_y:dead[0][1] mode:mode];
        
        now_tech = @"🐡🐡";
        return 1;
    }
    
    // Rank 2 attack
    for (int i = border_x_min; i < border_x_max + 1; i++)
        for (int j = border_y_min; j < border_y_max; j++)
        {
            if ([self killpoint:i y:j mode:mode] == 1)
            {
                [self add_a_chess:i pl_y:j mode:mode];
                now_tech = @"🕊🕊";
                return 1;
            }
        }
    
    // Rank 2 pre attack and defense
    int fast_attack [100][2];
    int fast_res = [self harsh_fast_attack_point:fast_attack mode:mode];
    if (fast_res > 0)
    {
        int high_score = -1000000;
        int high_id = -1;
        for (int i = 0; i < fast_res; i++)
        {
            // if (fight_back_level(fast_attack[i, 0], fast_attack[i, 1], mode) != 0)
            //   continue;
            
            int current_score = [self point_score:fast_attack[i][0] y:fast_attack[i][1] mode:mode];
            if (current_score > high_score)
            {
                high_score = current_score;
                high_id = i;
            }
        }
        if (high_id != -1)
        {
            [self add_a_chess:fast_attack[high_id][0] pl_y:fast_attack[high_id][1] mode:mode];
            
            now_tech = @"🦐🦐";
            return 1;
        }
        
    }
    
    // Rank 2 defense
    int need_defense [100][2];
    int enemy_force [100][2];
    int enemy_force_total = [self dec_force_point:enemy_force mode: -mode];
    int need_defense_total = [self need_point:need_defense mode: mode];
    
    if (enemy_force_total > 0)
    {
        if (need_defense_total == 0)
            need_defense_total = [self dec_need_point:need_defense mode:mode];
        
        if (need_defense_total == 0)
        {
            int guess_defense_id = rand()% enemy_force_total;
            int dinger = -1;
            for (int i = 0; i < enemy_force_total; i++)
                if ([self keypoint:enemy_force[i][0] y:enemy_force[i][1] mode:mode type:0] != 0)
                {
                    dinger = i;
                    break;
                }
            if (dinger == -1)
                [self add_a_chess:enemy_force[guess_defense_id][0] pl_y:enemy_force[guess_defense_id][1] mode:mode];
            else
            {
                [self add_a_chess:enemy_force[dinger][0] pl_y:enemy_force[dinger][1] mode:mode];
            }
            now_tech = @"🦀🦀";
            return 1;
        }
        int high_id = -1;
        int high_score = -100000;
        
        for (int p = 0; p < need_defense_total; p++)
        {
            
            int current_score = [self point_score:need_defense[p][0] y:need_defense[p][1] mode:mode];
            
            if (current_score > high_score)
            {
                high_id = p;
                high_score = current_score;
            }
        }
        if (high_id == -1)
            high_id = rand()% need_defense_total;
        
        if (banned_mode == 1 && mode == 1)
        {
            if ([self banned_point:need_defense[high_id][0] j:need_defense[high_id][1]] == 1)
            {
                game_end = -1;
            }
        }
        [self add_a_chess:need_defense[high_id][0] pl_y:need_defense[high_id][1] mode:mode];
        now_tech = [[NSString alloc]initWithFormat: @"🐬🐬"];
        return 1;
    }
    
    return 0;
}
-(void) update_border
{
    int xmax=-100;
    int xmin=100;
    int ymax=-100;
    int ymin=100;
    
    for(int i=0;i<maxchessline;i++)
    {
        for(int j=0;j<maxchessline;j++)
        {
            if(chessboard[i][j]!=0)
            {
                if(i>xmax)
                    xmax=i;
                if(j>ymax)
                    ymax=j;
                if(i<xmin)
                    xmin=i;
                if(j<ymin)
                    ymin=j;
            }
        }
    }
    border_x_max=xmax+2;
    if(border_x_max>maxchessline-1)
        border_y_max=maxchessline-1;
    border_y_max=ymax+2;
    if(border_y_max>maxchessline-1)
        border_y_max=maxchessline-1;
    
    border_x_min=xmin-2;
    if(border_x_min<0)
        border_x_min=0;
    border_y_min=ymin-2;
    if(border_y_min<0)
        border_y_min=0;
}

-(int)Abs:(int)x
{
    if(x<0)
        return -x;
    return x;
}
-(int) bad_start:(int) i j:( int )j chessid: (int) chessid
{
    int aim = 0;
    if (chessid == 2 || chessid == 3)
    {
        aim = 3;
    }
    if (chessid == 4 || chessid == 5)
    {
        aim = 4;
    }
    if (aim == 0)
        return 0;
    
    for (int x = border_x_min; x < border_x_max + 1; x++)
        for (int y = border_y_min; y < border_y_max + 1; y++)
        {
            if (x < 0 || x > 14 || y < 0 || y > 14)
                continue;
            if (chessboard[x][ y] != 0)
            {
                
                if ( x - i>= aim ||y - j >= aim||i-x>=aim||j-y>=aim)
                    return 1;
            }
        }
    return 0;
}

//imagine
-(int) imagine:(int)x_id y:(int)y_id mode:(int) mode tower:(int) tower;
{
    if (chessboard[x_id][y_id] != 0)
        return 0;
    
    if ([self fight_back_level:x_id y:y_id mode:mode] != 0)
        return 0;
    
    if ([self harsh_four_hide_attack:x_id y:y_id mode:mode tower:tower] == 1)
    {
        return 1;
    }
    if ([self harsh_doublethree_hide_attack:x_id y:y_id mode:mode tower:tower]  == 1)
    {
        return 1;
    }
    return 0;
}
-(int) advance_doublethree_hide_attack:(int) x y:(int) y mode:(int) mode tower:(int) tower;
{
    if(x<0||x>maxchessline-1||y<0||y>maxchessline-1||tower>=5)
        return 0;
    int rival_have_to_go[7]={};
    
    int threeler=[self doublethreetest_save:x y:y mode:mode savers:rival_have_to_go border:15];
    if(threeler==0)
        return 0;
    
    int res=rival_have_to_go[6];
    chessboard[x][y]=mode;
    
    if(res==0)
    {
        chessboard[x][y]=0;
        return 0;
    }
    
    int defense_no_use_num=0;
    for(int p=0;p<res;p++)
    {
        //   cout<<"("<<tower<<") "<<"rival may go to "<<rival_have_to_go[p*2]<<","<<rival_have_to_go[p*2+1]<<endl;
        chessboard[rival_have_to_go[p*2]][rival_have_to_go[p*2+1]]=-mode;
        if([self exist_force:-mode]==1)
        {
            chessboard[rival_have_to_go[p*2]][rival_have_to_go[p*2+1]]=0;
            break;
        }
        
        int couldnot=0;
        for(int i=x-3;i<x+4&&couldnot==0;i++)
        {
            if(i>maxchessline-1||i<0)
                continue;
            for(int j=y-3;j<y+4;j++)
            {
                if(j>maxchessline-1||j<0||j==y&&i==x)
                    continue;
                if(!(i==x||j==y||i-x==j-y||i-x==y-j))
                    continue;
                if(chessboard[i][j]!=0)
                    continue;
                
                int kill_val =[self killpoint:i y:j mode:mode];
                if(kill_val==2)
                {
                    int fightbacker=[self fight_back_level:i y:j mode:mode];
                    if(fightbacker==0)
                    {
                        couldnot=1;
                        break;
                    }
                }
                if(kill_val==1)
                {
                    couldnot = 1;
                    break;
                }
                if([self advance_four_hide_attack:i y:j mode:mode tower:tower+1] !=0||[self advance_doublethree_hide_attack:i y:j mode:mode tower:tower+1] !=0)
                {
                    couldnot=1;
                    break;
                }
            }
        }
        if(couldnot==1)
            defense_no_use_num++;
        
        chessboard[rival_have_to_go[p*2]][rival_have_to_go[p*2+1]]=0;
    }
    chessboard[x][y]=0;
    if(defense_no_use_num==res)
        return 1;
    
    return 0;
}
-(int) advance_super_fast_attack:(int) x y:(int) y mode:(int) mode tower:(int) tower;
{
    if(x<0||x>maxchessline-1||y<0||y>maxchessline-1||tower>=5)
        return 0;
    int rival_have_to_go[2]={};
    int res=[self keypoint_save:x y:y mode:mode aimer:rival_have_to_go];
    if(res==0)
        return 0;
    
    chessboard[x][y]=mode;
    chessboard[rival_have_to_go[0]][rival_have_to_go[1]]=-mode;
    int couldnot=0;
    for(int i=x-3;i<x+4&&couldnot==0;i++)
    {
        if(i>maxchessline-1||i<0)
            continue;
        for(int j=y-3;j<y+4;j++)
        {
            if(j>maxchessline-1||j<0||j==y||i==x)
                continue;
            if(!(i==x||j==y||i-x==j-y||i-x==y-j))
                continue;
            if(chessboard[i][j]!=0)
                continue;
            
            int kill_val = [self killpoint:i y:j mode:mode];
            if(kill_val==2)
            {
                int fightbacker=[self fight_back_level:i y:j mode:mode];
                if(fightbacker==0)
                {
                    couldnot=1;
                    break;
                }
            }
            if(kill_val==1)
            {
                couldnot = 1;
                break;
            }
            if([self advance_super_fast_attack:i y:j mode:mode tower:tower+1]!=0)
            {
                couldnot=1;
                break;
            }
        }
    }
    
    chessboard[rival_have_to_go[0]][rival_have_to_go[1]]=0;
    chessboard[x][y]=0;
    return couldnot;
}
-(int) advance_four_hide_attack:(int) x y:(int) y mode:(int) mode tower:(int) tower;
{
    if(x<0||x>maxchessline-1||y<0||y>maxchessline-1||tower>=5)
        return 0;
    
    int rival_have_to_go[2]={};
    int res=[self keypoint_save:x y:y mode:mode aimer:rival_have_to_go];
    
    if(res==0)
        return 0;
    
    chessboard[x][y]=mode;
    chessboard[rival_have_to_go[0]][rival_have_to_go[1]]=-mode;
    int couldnot=0;
    for(int i=x-3;i<x+4&&couldnot==0;i++)
    {
        if(i>maxchessline-1||i<0)
            continue;
        for(int j=y-3;j<y+4;j++)
        {
            if(j>maxchessline-1||j<0||j==y||i==x)
                continue;
            if(!(i==x||j==y||i-x==j-y||i-x==y-j))
                continue;
            if(chessboard[i][j]!=0)
                continue;
            
            int kill_val =[self killpoint:i y:j mode:mode];
            if(kill_val==2)
            {
                int fightbacker=[self fight_back_level:i y:j mode:mode];
                if(fightbacker==0)
                {
                    couldnot=1;
                    break;
                }
            }
            if(kill_val==1)
            {
                couldnot = 1;
                break;
            }
            if([self advance_four_hide_attack:i y:j mode:mode tower:tower+1] !=0||[self advance_doublethree_hide_attack:i y:j mode:mode tower:tower+1]!=0)
            {
                couldnot=1;
                break;
            }
        }
    }
    
    chessboard[rival_have_to_go[0]][rival_have_to_go[1]]=0;
    chessboard[x][y]=0;
    return couldnot;
}
-(void) export_current_board:(int[15][15])board
{
    for(int i=0;i<15;i++)
        for(int j=0;j<15;j++)
        {
            board[i][j]=chessboard[i][j];
        }
}
-(void) import_from_board:(int[15][15])board
{
    for(int i=0;i<15;i++)
        for(int j=0;j<15;j++)
        {
            chessboard[i][j]=board[i][j];
        }
}
-(int) win_state
{
    return game_end;
}
-(void) clear_all_data
{
    game_end = 0;
    for (int i = 0; i < totalprocess; i++)
    {
        process[i][0] = 0;
        process[i][1] = 0;
    }
    
    
    for (int i = 0; i < maxchessline; i++)
        for (int j = 0; j < maxchessline; j++)
        {
            chessboard[i][j] = 0;
        }
    
    A_win_time=0;
    B_win_time=0;
    total_train_time=0;
    moder_now=1;
    train_on=0;
    chessid=0;
    vsmode=1;
    game_end=0;
    maxid=0;
    border_x_min=100;
    border_x_max=-1;
    border_y_min=100;
    border_y_max=-1;
    for(int i=0;i<15;i++)
        for(int j=0;j<15;j++)
            chessboard[i][j]=0;
    for(int i=0;i<225;i++)
        for(int j=0;j<2;j++)
            process[i][j]=0;
    updater=0;
    
    maxchessline=15;
    border=15;
    totalprocess=0;
}
-(int) current_banmode
{
    return banned_mode;
}
-(void) withdraw_two_steps
{
    if(chessid>1)
    {
        if(game_end == -1)
        {
            for(int j=1;j<=1;j++)
                chessboard[process[chessid-j][0]][process[chessid-j][1]]=0;
            
            chessid-=1;
            game_end=0;
        }
        else{
            for(int j=1;j<=2;j++)
                chessboard[process[chessid-j][0]][process[chessid-j][1]]=0;
            
            chessid-=2;
            game_end=0;
        }
        
    }
}

-(int) harsh_doublethree_hide_attack:(int)x y:(int)y mode:(int)mode tower:(int)tower
{
    if (x < 0 || x > maxchessline - 1 || y < 0 || y > maxchessline - 1 || tower >= 8)
        return 0; // if tower is more than 7 which mean has calculate atmost 4^7 steps
    
    int rival_have_to_go[7] = {};
    int threeler = [self doublethreetest:x y:y mode:mode type:0 border:15];
    if (threeler == 0)
        return 0;
    
    [self doublethreetest_save:x y:y mode:mode savers:rival_have_to_go border:15];
    int res = rival_have_to_go[6];
    chessboard[x][y] = mode;
    if([self exist_force:-mode]==1)
    {
        chessboard[x][y] = 0;
        return 0;
    }
    for (int p = 0; p < res; p++) // break current imagine process
    {
        if ([self keypoint:rival_have_to_go[p*2] y:rival_have_to_go[p*2+1] mode:-mode type:0] != 0)
        {
            
            if ([self killpoint:rival_have_to_go[p*2] y:rival_have_to_go[p*2]+1 mode:-mode]!= 0)
            {
                chessboard[x][y] = 0;
                return 0;
            }
            
            int fighterback[2] = {};
            [self keypoint_save:rival_have_to_go[p*2] y:rival_have_to_go[p*2+1] mode:-mode aimer:fighterback];
            // keypoint_save(rival_have_to_go[p * 2 + 0], rival_have_to_go[p * 2 + 1], -mode, fighterback);
            chessboard[rival_have_to_go[p * 2 + 0] ][ rival_have_to_go[p * 2 + 1]] = -mode;
            if ([self harsh_four_hide_attack:fighterback[0] y:fighterback[1] mode:mode tower:tower+1] == 1 ||
                [self harsh_doublethree_hide_attack:fighterback[0] y:fighterback[1] mode:mode tower:tower+1] == 1)
            {
                chessboard[x][y] = 0;
                chessboard[rival_have_to_go[p * 2 + 0] ][ rival_have_to_go[p * 2 + 1]] = 0;
                continue;
            }
            else
            {
                chessboard[x][y] = 0;
                chessboard[rival_have_to_go[p * 2 + 0] ][ rival_have_to_go[p * 2 + 1]] = 0;
                return 0;
            }
        }
        int doublethree_el = [self doublethreetest:rival_have_to_go[p*2] y:rival_have_to_go[p*2+1] mode:-mode type:0 border:15];
        
        if(doublethree_el!=0)
        {
            int fighterback[7] = {};
            [self doublethreetest_save:rival_have_to_go[p*2] y:rival_have_to_go[p*2+1] mode:-mode savers:fighterback border:15];
            chessboard[rival_have_to_go[p * 2 + 0]][rival_have_to_go[p * 2 + 1]] = -mode;
            if([self exist_force:mode]==1)
            {
                chessboard[rival_have_to_go[p * 2 + 0]][rival_have_to_go[p * 2 + 1]] = 0;
                continue;
            }
            int space_num = fighterback[6];
            int has_solution = 0;
            for(int j=0;j<space_num;j++)
            {
                if([self harsh_four_hide_attack:fighterback[j*2] y:fighterback[j*2+1] mode:mode tower:tower+1] == 1 || [self harsh_doublethree_hide_attack:fighterback[j*2] y:fighterback[j*2+1] mode:mode tower:tower+1] == 1)
                {
                    has_solution = 1;
                    break;
                }
            }
            chessboard[rival_have_to_go[p * 2 + 0] ][ rival_have_to_go[p * 2 + 1]] = 0;
            if(has_solution==0)
            {
                chessboard[x][y] = 0;
                return 0;
            }
        }
    }
    
    bool wented [3] = {};
    
    for (int s = 0; s < res; s++) // check if there is a way to break current process
    {
        int p = rand()%res;
        while (wented[p] == true)
            p = rand()%res;
        
        wented[p] = true;
        
        chessboard[rival_have_to_go[p * 2] ][ rival_have_to_go[p * 2 + 1]] = -mode;
        
        int couldnot = 0;
        for (int i = x - 3; i < x + 4 && couldnot == 0; i++)
        {
            if (i > maxchessline - 1 || i < 0)
                continue;
            for (int j = y - 3; j < y + 4; j++)
            {
                if (j > maxchessline - 1 || j < 0 || j == y && i == x)
                    continue;
                if (!(i == x || j == y || i - x == j - y || i - x == y - j))
                    continue;
                if (chessboard[i ][ j] != 0)
                    continue;
                
                int kill_val = [self killpoint:i y:j mode:mode];
                if (kill_val == 1)
                {
                    couldnot = 1;
                    break;
                }
                if ([self harsh_four_hide_attack:i y:j mode:mode tower:tower+1] != 0 || [self harsh_doublethree_hide_attack:i y:j mode:mode tower:tower+1] != 0)
                {
                    // check if there is a continue attack way, start by doublethree or four
                    couldnot = 1;
                    break;
                }
            }
        }
        if (couldnot == 0) // find a way to defense so return 0
        {
            chessboard[rival_have_to_go[p * 2] ][ rival_have_to_go[p * 2 + 1]] = 0;
            chessboard[x][y] = 0;
            return 0;
        }
        
        chessboard[rival_have_to_go[p * 2] ][ rival_have_to_go[p * 2 + 1]] = 0;
    }
    chessboard[x ][ y] = 0; // this step is undefensed so return 1
    return 1;
}

-(int) harsh_four_hide_attack:(int)x y:(int)y mode:(int)mode tower:(int)tower
{
    if (x < 0 || x > maxchessline - 1 || y < 0 || y > maxchessline - 1 || tower >= 8)
        return 0;
    
    int rival_have_to_go [2] = {};
    int res = [self keypoint_save:x y:y mode:mode aimer:rival_have_to_go];
    if (res == 0)
        return 0;
    else{
        if([self harsh_super_fast_attack:x y:y mode:mode tower:0]!=0)
            return 1;
    }
    
    chessboard[x][y] = mode;
    
    if ([self keypoint:rival_have_to_go[0] y:rival_have_to_go[1] mode:-mode type:0] != 0)
    {
        
        if ([self killpoint:rival_have_to_go[0] y:rival_have_to_go[1] mode:-mode] != 0)
        {
            chessboard[x][y] = 0;
            return 0;
        }
        
        int fighterback [2] = {};
        [self keypoint_save:rival_have_to_go[0] y:rival_have_to_go[1] mode:-mode aimer:fighterback];
        chessboard[rival_have_to_go[0] ][ rival_have_to_go[1]] = -mode;
        if ([self harsh_four_hide_attack:fighterback[0] y:fighterback[1] mode:mode tower:tower+1] != 0 ||
            [self harsh_doublethree_hide_attack:fighterback[0] y:fighterback[1] mode:mode tower:tower+1] != 0)
        {
            chessboard[x][y] = 0;
            chessboard[rival_have_to_go[0]][rival_have_to_go[1]] = 0;
            return 1;
        }
        else
        {
            chessboard[x][y] = 0;
            chessboard[rival_have_to_go[0]][rival_have_to_go[1]] = 0;
            return 0;
        }
    }
    
    chessboard[rival_have_to_go[0]][rival_have_to_go[1]] = -mode;
    int couldnot = 0;
    for (int i = x - 3; i < x + 4 && couldnot == 0; i++)
    {
        if (i > maxchessline - 1 || i < 0)
            continue;
        for (int j = y - 3; j < y + 4; j++)
        {
            if (j > maxchessline - 1 || j < 0 || j == y || i == x)
                continue;
            if (!(i == x || j == y || i - x == j - y || i - x == y - j))
                continue;
            if (chessboard[i][j] != 0)
                continue;
            
            int kill_val = [self harsh_super_fast_attack:i y:j mode:mode tower:0];
            if (kill_val == 1)
            {
                couldnot = 1;
                break;
            }
            
            if ([self harsh_four_hide_attack:i y:j mode:mode tower:tower+1] != 0 || [self harsh_doublethree_hide_attack:i y:j mode:mode tower:tower+1] != 0)
            {
                couldnot = 1;
                break;
            }
        }
    }
    
    chessboard[rival_have_to_go[0] ][ rival_have_to_go[1]] = 0;
    chessboard[x ][ y] = 0;
    return couldnot;
}
-(int) harsh_super_fast_attack:(int)x y:(int)y mode:(int)mode tower:(int)tower
{
    if (x < 0 || x > maxchessline - 1 || y < 0 || y > maxchessline - 1 || tower >= 10)
        return 0;
    
    int rival_have_to_go [2] = {};
    int res = [self keypoint_save:x y:y mode:mode aimer:rival_have_to_go];
    if (res == 0)
        return 0;
    else{
        if([self killpoint:x y:y mode:mode]!=0)
            return 1;
    }
    
    chessboard[x][y] = mode;
    
    if ([self keypoint:rival_have_to_go[0] y:rival_have_to_go[1] mode:-mode type:0] != 0)
    {
        
        if ([self killpoint:rival_have_to_go[0] y:rival_have_to_go[1] mode:-mode] != 0)
        {
            chessboard[x][y] = 0;
            return 0;
        }
        
        int fighterback [2]={};
        [self keypoint_save:rival_have_to_go[0] y:rival_have_to_go[1] mode:-mode aimer:fighterback];
        chessboard[rival_have_to_go[0] ][ rival_have_to_go[1]] = -mode;
        if ([self harsh_super_fast_attack:fighterback[0] y:fighterback[1] mode:mode tower:tower+1] == 1)
        {
            //print("super fight back ");
            chessboard[x][y] = 0;
            chessboard[rival_have_to_go[0] ][ rival_have_to_go[1]] = 0;
            return 1;
        }
        else
        {
            chessboard[x][y] = 0;
            chessboard[rival_have_to_go[0] ][ rival_have_to_go[1]] = 0;
            return 0;
        }
    }
    
    chessboard[rival_have_to_go[0] ][rival_have_to_go[1]] = -mode;
    
    int couldnot = 0;
    for (int i = x - 3; i < x + 4 && couldnot == 0; i++)
    {
        if (i > maxchessline - 1 || i < 0)
            continue;
        for (int j = y - 3; j < y + 4; j++)
        {
            if (j > maxchessline - 1 || j < 0 || j == y || i == x)
                continue;
            if (!(i == x || j == y || i - x == j - y || i - x == y - j))
                continue;
            if (chessboard[i][j] != 0)
                continue;
            
            int kill_val = [self killpoint:i y:j mode:mode];
            if (kill_val == 1)
            {
                couldnot = 1;
                break;
            }
            
            if ([self harsh_super_fast_attack:i y:j mode:mode tower:tower+1] != 0)
            {
                couldnot = 1;
                break;
            }
        }
    }
    
    chessboard[rival_have_to_go[0]][rival_have_to_go[1]] = 0;
    chessboard[x][y] = 0;
    return couldnot;
}
-(int)get_last_pos_return_color:(int[2])pos
{
    pos[0]=last_pos[0];
    pos[1]=last_pos[1];
    return last_color;
}
-(int)export_stack:(int[225][2])stacker
{
    for(int i=0;i<chessid;i++)
        for(int j=0;j<2;++j)
            stacker[i][j]=process[i][j];
    return chessid;
}
-(void)import_stack:(int[225][2])stacker height:(int)stack_height
{
    chessid=stack_height;
    for(int i=0;i<chessid;i++)
        for(int j=0;j<2;++j)
            process[i][j]=stacker[i][j];
    
    last_pos[0]=process[stack_height-1][0];
    last_pos[1]=process[stack_height-1][1];
    if(stack_height%2==1)
    {
        last_color=1;
    }
    else{
        last_color=-1;
    }
}
-(NSString*)get_now_tech
{
    return now_tech;
}
-(void) teaching_current_step:(int[15][15])paint_map
{
    if(game_end!=0)
        return;
    int origin_map[15][15];
    int origin_last_pos[2];
    int origin_last_color=last_color;
    int origin_id = chessid;
    for(int q=0;q<2;q++)
        origin_last_pos[q]=last_pos[q];
    
    for(int i=0;i<15;i++)
        for(int j=0;j<15;j++)
        {
            paint_map[i][j]=0;
            origin_map[i][j]=chessboard[i][j];
        }
    int analy_moder = last_color;
    int ider_now=1;
    paint_map[last_pos[0]][last_pos[1]]=ider_now*analy_moder;
    for(ider_now=2;ider_now<15&&game_end==0;ider_now++)
    {
        analy_moder*=-1;
        [self easy_analysisboard:analy_moder];
        // NSLog(@"next is %d,%d color %d",last_pos[0],last_pos[1],last_color);
        paint_map[last_pos[0]][last_pos[1]]=ider_now*analy_moder;
    }
    
    for(int i=0;i<15;i++)
        for(int j=0;j<15;j++)
        {
            chessboard[i][j]=origin_map[i][j];
        }
    for(int q=0;q<2;q++)
        last_pos[q]=origin_last_pos[q];
    last_color=origin_last_color;
    chessid=origin_id;
    game_end=0;
}
@end

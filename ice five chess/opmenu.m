//
//  opmenu.m
//  ice sudoku
//
//  Created by 王子诚 on 2019/7/24.
//  Copyright © 2019 王子诚. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "opmenu.h"

@interface opmenu()

@end

@implementation opmenu
-(void)viewDidLoad
{
    [super viewDidLoad];
    CGRect Screensize=[UIScreen mainScreen].bounds;
    long ScreenWidth=Screensize.size.width;
    long ScreenHeight=Screensize.size.height;
    long area_all=ScreenHeight*ScreenWidth;
    if(area_all>500000)//ipad
    {
        UIStoryboard *mainStoryboard=[UIStoryboard storyboardWithName:@"HDMain" bundle:nil];
        UIViewController *editorVC=[mainStoryboard instantiateViewControllerWithIdentifier:@"ipadview"];
        editorVC.modalTransitionStyle=UIModalTransitionStyleCrossDissolve;
        editorVC.modalPresentationStyle=UIModalPresentationFullScreen;
        [self presentViewController:editorVC animated:NO completion:nil];
        return;
    }
    else{
        UIStoryboard *mainStoryboard=[UIStoryboard storyboardWithName:@"Main" bundle:nil];
        UIViewController *editorVC=[mainStoryboard instantiateViewControllerWithIdentifier:@"iphoneview"];
        editorVC.modalTransitionStyle=UIModalTransitionStyleCrossDissolve;
        editorVC.modalPresentationStyle=UIModalPresentationFullScreen;
        [self presentViewController:editorVC animated:NO completion:nil];
    }
    
}

@end

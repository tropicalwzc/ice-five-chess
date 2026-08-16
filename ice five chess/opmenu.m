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
@property (assign, nonatomic) BOOL gameViewPresented;
@end

@implementation opmenu

- (void)viewDidLoad
{
    [super viewDidLoad];
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];

    // The initial controller is loaded before its view is attached to the
    // window. Presenting here avoids presenting a navigation controller from
    // a detached `opmenu` view controller.
    if (self.gameViewPresented || !self.isViewLoaded || self.view.window == nil || self.presentedViewController != nil) {
        return;
    }

    CGRect Screensize=[UIScreen mainScreen].bounds;
    long ScreenWidth=Screensize.size.width;
    long ScreenHeight=Screensize.size.height;
    long area_all=ScreenHeight*ScreenWidth;
    UIViewController *editorVC = nil;
    if(area_all>500000)//ipad
    {
        UIStoryboard *mainStoryboard=[UIStoryboard storyboardWithName:@"HDMain" bundle:nil];
        editorVC=[mainStoryboard instantiateViewControllerWithIdentifier:@"ipadview"];
    }
    else{
        UIStoryboard *mainStoryboard=[UIStoryboard storyboardWithName:@"Main" bundle:nil];
        editorVC=[mainStoryboard instantiateViewControllerWithIdentifier:@"iphoneview"];
    }

    editorVC.modalTransitionStyle=UIModalTransitionStyleCrossDissolve;
    editorVC.modalPresentationStyle=UIModalPresentationFullScreen;
    self.gameViewPresented = YES;
    [self presentViewController:editorVC animated:NO completion:nil];
}

@end

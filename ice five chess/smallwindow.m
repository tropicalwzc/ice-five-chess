//
//  smallwindow.m
//  ice_checker
//
//  Created by 王子诚 on 2019/2/19.
//  Copyright © 2019 王子诚. All rights reserved.
//


#import "smallwindow.h"

@interface smallwindow()

@end

@implementation smallwindow
-(smallwindow*)init_with_fatherwindow:(UIViewController*)aim;
{
    father_window=aim;
    return self;
}
-(void)Simple_NewsMessage_With_Title:(NSString*)title andMessage:(NSString*)message{
    UIAlertController * alert = [UIAlertController alertControllerWithTitle:title message: message preferredStyle:UIAlertControllerStyleActionSheet];
    
    UIAlertAction * action = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleCancel handler:nil];
    [alert addAction:action];
    [self->father_window presentViewController: alert animated: YES completion:nil];
}
-(void)Simple_alertMessage_With_Title:(NSString*)title andMessage:(NSString*)message{
    UIAlertController * alert = [UIAlertController alertControllerWithTitle:title message: message preferredStyle:UIAlertControllerStyleAlert];
    
    UIAlertAction * action = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleCancel handler:nil];
    [alert addAction:action];
    [self->father_window presentViewController: alert animated: YES completion:nil];
}

-(void)set_father_view:(UIViewController*) aim;
{
    father_window=aim;
}
-(void)Input_one_line_window_with_title:(NSString*)title andMessage:(NSString*)message placeholder:(NSString*)placehoderstr process_NSString_func:(SEL)func
{
    UIAlertController* alert = [UIAlertController alertControllerWithTitle:title message:message preferredStyle:UIAlertControllerStyleAlert];
    [alert addTextFieldWithConfigurationHandler:^(UITextField *textField) {
        textField.placeholder = placehoderstr; // if needs
        if(textField.placeholder.length==81)
        {
            textField.text=placehoderstr;
        }
    }];
    UIAlertAction * action_cancle = [UIAlertAction actionWithTitle:@"Cancle" style:UIAlertActionStyleCancel handler:nil];
    UIAlertAction *submit = [UIAlertAction actionWithTitle:@"Done" style:UIAlertActionStyleDefault
                                                   handler:^(UIAlertAction * action) {
                                                       UITextField *textField = [alert.textFields firstObject];
                                                       [self->father_window performSelector:func withObject:textField.text afterDelay:0];
                                                   }];
    [alert addAction:submit];
    [alert addAction:action_cancle];
    [father_window presentViewController:alert animated:YES completion:nil];
}
-(void)Sure_or_not_window_with_title:(NSString*)title andMessage:(NSString*)message process_func:(SEL)func sure_button_title: (NSString*)sure_text cancle_button_title:(NSString*)cancle_text
{
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:title message:message preferredStyle:UIAlertControllerStyleAlert];
    // 确定注销
    UIAlertAction* okAction = [UIAlertAction actionWithTitle:sure_text style:UIAlertActionStyleDestructive handler:^(UIAlertAction *_Nonnull action) {
        [self->father_window performSelector:func withObject:nil afterDelay:0];
    }];
    UIAlertAction* cancelAction =[UIAlertAction actionWithTitle:cancle_text style:UIAlertActionStyleCancel handler:nil];
    [alert addAction:okAction];
    [alert addAction:cancelAction];
    // 弹出对话框
    [father_window presentViewController:alert animated:YES completion:nil];
}
-(void)Rich_NewsMessage_with_two_button:(NSString*)title message:(NSString*)message button_nameset:(NSArray*)button_names funct1:(SEL)funct_one funct2:(SEL)funct_two;
{
    UIAlertController * alert = [UIAlertController alertControllerWithTitle:title message: message preferredStyle:UIAlertControllerStyleActionSheet];
    
    UIAlertAction *cancle_action = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleCancel handler:nil];
    [alert addAction:cancle_action];

        UIAlertAction *first_action = [UIAlertAction actionWithTitle:button_names[0] style:UIAlertActionStyleDefault handler:^(UIAlertAction *_Nonnull action) {
            [self->father_window performSelector:funct_one withObject:nil afterDelay:0];
        }];
        [alert addAction:first_action];
    
    UIAlertAction *second_action = [UIAlertAction actionWithTitle:button_names[1] style:UIAlertActionStyleDefault handler:^(UIAlertAction *_Nonnull action) {
        [self->father_window performSelector:funct_two withObject:nil afterDelay:0];
    }];
    [alert addAction:second_action];
    
    [self->father_window presentViewController: alert animated: YES completion:nil];
}
-(void)Rich_NewsMessage_with_three_button:(NSString*)title message:(NSString*)message button_nameset:(NSArray*)button_names funct1:(SEL)funct_one funct2:(SEL)funct_two funct3:(SEL)funct_three
{
    UIAlertController * alert = [UIAlertController alertControllerWithTitle:title message: message preferredStyle:UIAlertControllerStyleActionSheet];
    
    UIAlertAction *cancle_action = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleCancel handler:nil];
    [alert addAction:cancle_action];
    
    UIAlertAction *first_action = [UIAlertAction actionWithTitle:button_names[0] style:UIAlertActionStyleDefault handler:^(UIAlertAction *_Nonnull action) {
        [self->father_window performSelector:funct_one withObject:nil afterDelay:0];
    }];
    [alert addAction:first_action];
    
    UIAlertAction *second_action = [UIAlertAction actionWithTitle:button_names[1] style:UIAlertActionStyleDefault handler:^(UIAlertAction *_Nonnull action) {
        [self->father_window performSelector:funct_two withObject:nil afterDelay:0];
    }];
    [alert addAction:second_action];
    
    UIAlertAction *third_action = [UIAlertAction actionWithTitle:button_names[2] style:UIAlertActionStyleDestructive handler:^(UIAlertAction *_Nonnull action) {
        [self->father_window performSelector:funct_three withObject:nil afterDelay:0];
    }];
    [alert addAction:third_action];
    
    [self->father_window presentViewController: alert animated: YES completion:nil];
}
-(void)Rich_NewsMessage_with_four_button:(NSString*)title message:(NSString*)message button_nameset:(NSArray*)button_names funct1:(SEL)funct_one funct2:(SEL)funct_two funct3:(SEL)funct_three funct4:(SEL)funct_four
{
    UIAlertController * alert = [UIAlertController alertControllerWithTitle:title message: message preferredStyle:UIAlertControllerStyleActionSheet];
    
    UIAlertAction *cancle_action = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleCancel handler:nil];
    [alert addAction:cancle_action];
    
    UIAlertAction *first_action = [UIAlertAction actionWithTitle:button_names[0] style:UIAlertActionStyleDefault handler:^(UIAlertAction *_Nonnull action) {
        [self->father_window performSelector:funct_one withObject:nil afterDelay:0];
    }];
    [alert addAction:first_action];
    
    UIAlertAction *second_action = [UIAlertAction actionWithTitle:button_names[1] style:UIAlertActionStyleDefault handler:^(UIAlertAction *_Nonnull action) {
        [self->father_window performSelector:funct_two withObject:nil afterDelay:0];
    }];
    [alert addAction:second_action];
    
    UIAlertAction *third_action = [UIAlertAction actionWithTitle:button_names[2] style:UIAlertActionStyleDefault
        handler:^(UIAlertAction *_Nonnull action) {
        [self->father_window performSelector:funct_three withObject:nil afterDelay:0];
    }];
    [alert addAction:third_action];
    
    UIAlertAction *fourth_action = [UIAlertAction actionWithTitle:button_names[3] style:UIAlertActionStyleDestructive handler:^(UIAlertAction *_Nonnull action) {
        [self->father_window performSelector:funct_four withObject:nil afterDelay:0];
    }];
    [alert addAction:fourth_action];
    
    [self->father_window presentViewController: alert animated: YES completion:nil];
}
@end

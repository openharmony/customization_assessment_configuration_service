/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import hilog from '@ohos.hilog';
import UIAbilityContext from 'application.UIAbilityContext';

export type onBeginCallback = (error: assessment.AssessmentError) => void;
export type onInterruptedCallback = (info: assessment.AssessmentInterruptInfo) => void;
export type onEndCallback = () => void;

const DOMAIN = 0xD003951;
const TAG = '[ANI_ASSESSMENT]';

export namespace assessment {
    static { loadLibrary('customization_assessment_ani.z'); }
    hilog.info(DOMAIN, TAG, 'load libcustomization_assessment_ani.z.so finish');

    let nativeObj: long = 0;  // the address of AniAssessment of cpp
    export function setNativeObj(nativeObject: long): void {
        nativeObj = nativeObject;
    }

    export enum AssessmentErrorCode {
        OK = 0,
        USER_CANCEL = 1,
        TIMEOUT = 2,
        SYSTEM_ERROR = 3,
        ENV_ANOMALY = 4,
    }

    export interface AssessmentConfig {
        duration: int;
        allowedApps: Array<string>;
    }

    export class AssessmentConfigInner implements AssessmentConfig {
        duration: int = 0;
        allowedApps: Array<string> = [];
    }

    export interface AssessmentError {
        code: AssessmentErrorCode;
        message?: string;
    }

    export interface AssessmentInterruptInfo {
        code: AssessmentErrorCode;
        message: string;
    }

    export interface IAssessmentCallback {
        onBegin(error: AssessmentError): void;
        onInterrupted(info: AssessmentInterruptInfo): void;
        onEnd(): void;
    }

    export native function isActive(): boolean;

    export native function getConfiguration(): AssessmentConfig;

    export native function end(context: UIAbilityContext): void;

    export native function beginSync(nativeObj: long,
                                     context: UIAbilityContext,
                                     config: AssessmentConfig,
                                     beginCallback: onBeginCallback,
                                     interruptedCallback: onInterruptedCallback,
                                     endCallback: onEndCallback): void;

    function begin(context: UIAbilityContext, config: AssessmentConfig, callback: IAssessmentCallback): void {
        if (!nativeObj) {
            hilog.error(DOMAIN, TAG, '@ohos.customization.assessment.ets, this.nativeObj is 0, return');
            return;
        }
        const beginCallback: onBeginCallback = callback.onBegin;
        const interruptedCallback: onInterruptedCallback = callback.onInterrupted;
        const endCallback: onEndCallback = callback.onEnd;
        beginSync(nativeObj, context, config, beginCallback, interruptedCallback, endCallback);
    }

    function runBeginEvent(cb: object, code: int, message: string): void {
        hilog.debug(DOMAIN, TAG, 'runBeginEvent called');
        const func = cb as onBeginCallback;
        const param: AssessmentError = {
            code: AssessmentErrorCode.fromValue(code),
            message: message,
        };
        func(param);
    }
    
    function runInterruptedEvent(cb: object, code: int, message: string): void {
        hilog.debug(DOMAIN, TAG, 'runInterruptedEvent called');
        const func = cb as onInterruptedCallback;
        const info: AssessmentInterruptInfo = {
            code: AssessmentErrorCode.fromValue(code),
            message: message,
        };
        func(info);
    }
    
    function runEndEvent(cb: object): void {
        hilog.debug(DOMAIN, TAG, 'runEndEvent called');
        const func = cb as onEndCallback;
        func();
    }
}  // namespace assessment
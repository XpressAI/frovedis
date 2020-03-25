
// Copyright (c) 2010-2019 niXman (i dot nixman dog gmail dot com). All
// rights reserved.
//
// This file is part of YAS(https://github.com/niXman/yas) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//
// Boost Software License - Version 1.0 - August 17th, 2003
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef __yas__tests__base__include__wstring_hpp
#define __yas__tests__base__include__wstring_hpp

/***************************************************************************/

template<typename archive_traits>
bool wstring_test(std::ostream &log, const char *archive_type, const char *test_name) {

	std::vector<std::wstring> wcs_collection =  {
		L"A E I O U c C d D p P Á É Í Ó Ú ç Ç d D p P à è ì ò ù ã õ â ê î ô û", // pt-br
		L"đái dầm chửa hoang bí mật bầy tôi", // vn
		L"我能吞下玻璃而不伤身体。", // zh-cn
		L"私はガラスを食べられます。それは私を傷つけません。", // ja
		L"मैं काँच खा सकता हूँ, मुझे उस से कोई पीडा नहीं होती.", // hi
		L"Я могу есть стекло, оно мне не вредит.", // ru
		L"나는 유리를 먹을 수 있어요. 그래도 아프지 않아요", // ko
		L"I can eat glass and it doesn't hurt me.", // en
		L"ฉันกินกระจกได้ แต่มันไม่ทำให้ฉันเจ็บ", // th
		L"أنا قادر على أكل الزجاج و هذا لا يؤلمني", // ar
		L"לְהַגִּיד בַּבֹּקֶר חַסְדֶּךָ וֶאֱמוּנָתְךָ בַּלֵּילוֹת", // he
		L"Μπορώ να φάω σπασμένα γυαλιά χωρίς να πάθω τίποτα.", // el
		L"Mohu jíst sklo, neublíží mi.", // cs
		L"Мога да ям стъкло, то не ми вреди.", // bg
		L"Cam yiyebilirim, bana zararı dokunmaz.", // tr
		L"Je peux manger du verre, ça ne me fait pas de mal.", // fr
	};

	for (auto& ws : wcs_collection) {
		std::wstring wss;

		typename archive_traits::oarchive oa;
		archive_traits::ocreate(oa, archive_type);
		oa& YAS_OBJECT_NVP("obj", ("ws", ws));

		typename archive_traits::iarchive ia;
		archive_traits::icreate(ia, oa, archive_type);
		ia& YAS_OBJECT_NVP("obj", ("ws", wss));

		if (ws != wss) {
			YAS_TEST_REPORT(log, archive_type, test_name);
			return false;
		}
	}

	return true;
}

/***************************************************************************/

#endif // __yas__tests__base__include__wstring_hpp
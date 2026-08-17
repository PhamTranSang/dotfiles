# Firefox chrome tweaks

Custom `userChrome.css` cho Firefox — hiện đang **đưa thanh tab xuống đáy cửa
sổ** (URL/address bar giữ nguyên ở trên).

## Có gì ở đây

| File | Vai trò |
| --- | --- |
| `user.js` | Bật `toolkit.legacyUserProfileCustomizations.stylesheets` để Firefox chịu đọc `userChrome.css`. |
| `chrome/userChrome.css` | CSS giao diện: ghim `#TabsToolbar` xuống đáy, chừa chỗ cho nội dung web. |

## Cài

`install-dotfiles.sh` tự dò profile trong `~/.mozilla/firefox/*.default-release`
(và `*.default`) rồi symlink `user.js` + `chrome/userChrome.css` vào đó. Vì tên
profile có tiền tố ngẫu nhiên nên không thể symlink vào path cố định như các app
khác.

Sau khi cài, **thoát hẳn Firefox rồi mở lại** (đóng cửa sổ thôi chưa đủ — `user.js`
chỉ nạp lúc khởi động).

## Chỉnh

- Cao/thấp thanh tab: sửa `--uc-tabbar-height` trong `chrome/userChrome.css`.
- Web bị che dải ở đáy → tăng giá trị đó; còn khe trống → giảm.
- Bỏ hẳn: xoá `chrome/userChrome.css` (hoặc cả `user.js`) rồi restart.

> Lưu ý: `userChrome.css` "đục" vào DOM giao diện Firefox nên **nhạy theo version**
> — bản Firefox mới có thể đổi selector, cần chỉnh lại. Đang chuẩn cho Firefox 153.

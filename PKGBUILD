# Maintainer: Nicolas Diaz <nico.d5155@outlook.com.ar>
pkgname=omado-git
pkgver=1.1.0.r19.1b9aa55
pkgrel=1
pkgdesc="Ultra-lightweight native task manager with Microsoft To Do sync for Omarchy"
arch=('x86_64')
url="https://github.com/nicodiaz101/omado"
license=('GPL3')
depends=(
    'qt6-base'
    'qt6-declarative'
    'qt6-svg'
    'qtkeychain-qt6'
)
makedepends=(
    'git'
    'cmake'
    'ninja'
    'gcc'
)
provides=('omado')
conflicts=('omado')
source=("git+https://github.com/nicodiaz101/omado.git")
sha256sums=('SKIP')

pkgver() {
    cd "$srcdir/omado"
    printf "1.1.0.r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

build() {
    cd "$srcdir/omado"
    cmake -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr
    cmake --build build
}

package() {
    cd "$srcdir/omado"
    DESTDIR="$pkgdir" cmake --install build
    # Ensure daemon desktop file is never placed in applications
    rm -f "$pkgdir/usr/share/applications/omado-daemon.desktop"
}

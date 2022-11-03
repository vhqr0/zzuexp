from django.urls import path

from .views import IndexView, AboutView, LoginView, SigninView, SigninVerifyView, LogoutView,\
    UserListView, UserDetailView, ProfileUpdateView, avatar, AvatarUploadView, \
    CategoryListView, CategoryDetailView, CategoryAddView, PageAddView

app_name = 'indexer'
urlpatterns = [
    path('', IndexView.as_view(), name='index'),
    path('about/', AboutView.as_view(), name='about'),
    path('login/', LoginView.as_view(), name='login'),
    path('signin/', SigninView.as_view(), name='signin'),
    path('signin-verify/<int:pk>/',
         SigninVerifyView.as_view(),
         name='signin-verify'),
    path('logout/', LogoutView.as_view(), name='logout'),
    path('user/', UserListView.as_view(), name='user-list'),
    path('user/<int:pk>/', UserDetailView.as_view(), name='user-detail'),
    path('profile-update/', ProfileUpdateView.as_view(),
         name='profile-update'),
    path('avatar/<int:pk>/', avatar, name='avatar'),
    path('avatar-upload/', AvatarUploadView.as_view(), name='avatar-upload'),
    path('category/', CategoryListView.as_view(), name='category-list'),
    path('category/<int:pk>/',
         CategoryDetailView.as_view(),
         name='category-detail'),
    path('category-add/', CategoryAddView.as_view(), name='category-add'),
    path('page-add/', PageAddView.as_view(), name='page-add'),
]

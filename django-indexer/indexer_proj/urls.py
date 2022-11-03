from django.shortcuts import redirect
from django.urls import path, include
from django.contrib import admin

urlpatterns = [
    path('', lambda request: redirect('indexer:index')),
    path('indexer/', include('indexer_app.urls')),
    path('admin/', admin.site.urls),
]
